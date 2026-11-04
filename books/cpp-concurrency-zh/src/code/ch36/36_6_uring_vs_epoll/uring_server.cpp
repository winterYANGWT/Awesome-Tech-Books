// kvd_uring：一个 io_uring 的键值服务器。
//
// 和前面章节的 kvd 用同一份 protocol/store，只把 I/O 层换掉：
//   - multishot accept：一次提交，后续每个新连接都产生一个 CQE；
//   - multishot recv + provided buffer ring：一次提交，每次数据到达都从
//     内核挑一个缓冲区并产生一个带 buffer id 的 CQE；
//   - user_data 里放“连接 id + 操作类型”，CQE 回来时据此找回状态。
#include <arpa/inet.h>
#include <liburing.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

namespace {

constexpr unsigned kRingEntries = 256;
constexpr unsigned kBufCount = 256;
constexpr unsigned kBufSize = 4096;
constexpr int kBufGroup = 1;

enum class Op : std::uint64_t { Accept = 0, Recv = 1, Send = 2, Close = 3 };

std::uint64_t encode(std::uint64_t conn, Op op) {
    return (conn << 8) | static_cast<std::uint64_t>(op);
}
std::uint64_t conn_of(std::uint64_t user_data) { return user_data >> 8; }
Op op_of(std::uint64_t user_data) {
    return static_cast<Op>(user_data & 0xff);
}

volatile sig_atomic_t g_stop = 0;
long g_syscalls = 0;
void on_signal(int) { g_stop = 1; }

struct Conn {
    int fd = -1;
    Parser parser;
    std::string out;      // 正在发送的这一段，CQE 回来前不能改
    std::string pending;  // 发送期间新产生的响应，等这一段发完再接上
    bool send_busy = false;
    bool close_pending = false;
};

struct Server {
    io_uring ring{};
    io_uring_buf_ring* buf_ring = nullptr;
    std::vector<std::unique_ptr<char[]>> buffers;
    std::unordered_map<std::uint64_t, std::unique_ptr<Conn>> conns;
    std::uint64_t next_conn = 1;
    Store store;

    bool init() {
        int ret = io_uring_queue_init(kRingEntries, &ring, 0);
        if (ret < 0) {
            std::fprintf(stderr, "io_uring_queue_init: %s\n", std::strerror(-ret));
            return false;
        }
        int err = 0;
        buf_ring = io_uring_setup_buf_ring(&ring, kBufCount, kBufGroup, 0, &err);
        if (buf_ring == nullptr) {
            std::fprintf(stderr, "io_uring_setup_buf_ring: %s\n", std::strerror(-err));
            return false;
        }
        buffers.reserve(kBufCount);
        for (unsigned i = 0; i < kBufCount; ++i) {
            buffers.emplace_back(new char[kBufSize]);
            io_uring_buf_ring_add(buf_ring, buffers.back().get(), kBufSize,
                                  static_cast<unsigned short>(i),
                                  io_uring_buf_ring_mask(kBufCount),
                                  static_cast<int>(i));
        }
        io_uring_buf_ring_advance(buf_ring, kBufCount);
        return true;
    }

    void submit_ring() {
        ++g_syscalls;
        io_uring_submit(&ring);
    }

    io_uring_sqe* get_sqe() {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (sqe == nullptr) {
            // SQ 满：先把已填的提交，再取一次。服务器里的提交都很短，
            // 所以这里只重试一次；失败说明环太小或内核没有推进。
            submit_ring();
            sqe = io_uring_get_sqe(&ring);
        }
        if (sqe == nullptr) std::fprintf(stderr, "SQ full\n");
        return sqe;
    }

    void arm_accept(int listener) {
        io_uring_sqe* sqe = get_sqe();
        if (sqe == nullptr) return;
        io_uring_prep_multishot_accept(sqe, listener, nullptr, nullptr, 0);
        io_uring_sqe_set_data64(sqe, encode(0, Op::Accept));
        submit_ring();
    }

    void arm_recv(std::uint64_t id, int fd) {
        io_uring_sqe* sqe = get_sqe();
        if (sqe == nullptr) return;
        io_uring_prep_recv_multishot(sqe, fd, nullptr, 0, 0);
        sqe->flags |= IOSQE_BUFFER_SELECT;
        sqe->buf_group = kBufGroup;
        io_uring_sqe_set_data64(sqe, encode(id, Op::Recv));
        submit_ring();
    }

    void return_buffer(unsigned short bid) {
        if (bid >= kBufCount) return;
        io_uring_buf_ring_add(buf_ring, buffers[bid].get(), kBufSize, bid,
                              io_uring_buf_ring_mask(kBufCount), 0);
        io_uring_buf_ring_advance(buf_ring, 1);
    }

    void submit_close(std::uint64_t id, Conn& c) {
        io_uring_sqe* sqe = get_sqe();
        if (sqe == nullptr) return;
        io_uring_prep_close(sqe, c.fd);
        io_uring_sqe_set_data64(sqe, encode(id, Op::Close));
        submit_ring();
    }

    void submit_send(std::uint64_t id, Conn& c) {
        if (c.out.empty()) {
            c.send_busy = false;
            if (c.close_pending) submit_close(id, c);
            return;
        }
        io_uring_sqe* sqe = get_sqe();
        if (sqe == nullptr) return;
        io_uring_prep_send(sqe, c.fd, c.out.data(), c.out.size(), MSG_NOSIGNAL);
        io_uring_sqe_set_data64(sqe, encode(id, Op::Send));
        c.send_busy = true;
        submit_ring();
    }

    // 有 pending 就先发 pending，否则若已要求关闭就关闭。
    void continue_conn(std::uint64_t id, Conn& c) {
        if (!c.pending.empty()) {
            c.out = std::move(c.pending);
            c.pending.clear();
            submit_send(id, c);
        } else if (c.close_pending) {
            submit_close(id, c);
        }
    }

    void want_close(std::uint64_t id, Conn& c) {
        if (c.close_pending) return;
        c.close_pending = true;
        if (!c.send_busy) continue_conn(id, c);
    }

    void on_accept(io_uring_cqe* cqe) {
        if (cqe->res < 0) {
            if (cqe->res != -ECANCELED)
                std::fprintf(stderr, "accept: %s\n", std::strerror(-cqe->res));
            return;
        }
        std::uint64_t id = next_conn++;
        auto c = std::make_unique<Conn>();
        c->fd = cqe->res;
        int fd = c->fd;
        conns.emplace(id, std::move(c));
        arm_recv(id, fd);
        // IORING_CQE_F_MORE 表示 multishot accept 还活着；最后一条没有它。
    }

    void on_recv(std::uint64_t id, io_uring_cqe* cqe) {
        auto it = conns.find(id);
        if (it == conns.end()) return;
        Conn& c = *it->second;

        if (cqe->res < 0) {
            if (cqe->res == -ENOBUFS) {
                // 缓冲区池暂时空了：multishot recv 结束，补上再重新装。
                if (!(cqe->flags & IORING_CQE_F_MORE)) arm_recv(id, c.fd);
            } else if (cqe->res != -ECANCELED) {
                want_close(id, c);
            }
            return;
        }
        if (cqe->res == 0) {  // 对端关了写方向
            want_close(id, c);
            return;
        }

        unsigned short bid = 0;
        bool has_buf = (cqe->flags & IORING_CQE_F_BUFFER) != 0;
        if (has_buf) bid = static_cast<unsigned short>(cqe->flags >> IORING_CQE_BUFFER_SHIFT);
        if (has_buf && bid < kBufCount) {
            std::string_view bytes(buffers[bid].get(), static_cast<std::size_t>(cqe->res));
            c.parser.feed(bytes);
            while (auto result = c.parser.next()) {
                if (auto* request = std::get_if<Request>(&*result)) {
                    c.pending += format_response(store.apply(*request));
                } else {
                    c.pending += format_response(std::get<ParseError>(*result));
                }
            }
            return_buffer(bid);
        }

        if (!(cqe->flags & IORING_CQE_F_MORE)) {
            // 正常的 multishot recv 在缓冲区耗尽或出错时结束；只要连接还在，
            // 就重新装一支。关闭中的连接不再装。
            if (!c.close_pending) arm_recv(id, c.fd);
        }
        if (!c.send_busy) continue_conn(id, c);
    }

    void on_send(std::uint64_t id, io_uring_cqe* cqe) {
        auto it = conns.find(id);
        if (it == conns.end()) return;
        Conn& c = *it->second;
        if (cqe->res < 0) {
            want_close(id, c);
            return;
        }
        c.out.erase(0, static_cast<std::size_t>(cqe->res));
        if (!c.out.empty()) {  // 短写：把剩下的再发一次
            submit_send(id, c);
            return;
        }
        c.send_busy = false;
        continue_conn(id, c);
    }

    void on_close(std::uint64_t id, io_uring_cqe* cqe) {
        auto it = conns.find(id);
        if (it == conns.end()) return;
        if (cqe->res < 0 && cqe->res != -ECANCELED)
            std::fprintf(stderr, "close: %s\n", std::strerror(-cqe->res));
        conns.erase(it);  // io_uring 的 close 已经把 fd 关掉
    }

    void handle(io_uring_cqe* cqe) {
        std::uint64_t user = io_uring_cqe_get_data64(cqe);
        std::uint64_t id = conn_of(user);
        switch (op_of(user)) {
        case Op::Accept: on_accept(cqe); break;
        case Op::Recv: on_recv(id, cqe); break;
        case Op::Send: on_send(id, cqe); break;
        case Op::Close: on_close(id, cqe); break;
        }
    }

    int run(int listener) {
        arm_accept(listener);
        while (!g_stop) {
            struct __kernel_timespec ts {};
            ts.tv_sec = 0;
            ts.tv_nsec = 100'000'000;  // 100 ms，只为定期检查 g_stop
            io_uring_cqe* cqe = nullptr;
            ++g_syscalls;
            int ret = io_uring_wait_cqe_timeout(&ring, &cqe, &ts);
            if (ret == -ETIME) continue;
            if (ret < 0) {
                if (ret == -EINTR) continue;
                std::fprintf(stderr, "wait_cqe: %s\n", std::strerror(-ret));
                break;
            }
            handle(cqe);
            io_uring_cqe_seen(&ring, cqe);
            // 顺手把已经排好的完成事件收完，减少进入内核的次数。
            while (io_uring_peek_cqe(&ring, &cqe) == 0) {
                handle(cqe);
                io_uring_cqe_seen(&ring, cqe);
            }
        }
        std::fprintf(stderr, "syscalls=%ld\n", g_syscalls);
        for (auto& [id, c] : conns) {
            (void)id;
            ::close(c->fd);
        }
        conns.clear();
        ::close(listener);
        io_uring_free_buf_ring(&ring, buf_ring, kBufCount, kBufGroup);
        io_uring_queue_exit(&ring);
        return 0;
    }
};

}  // namespace

int main(int argc, char** argv) {
    std::uint16_t port = 0;
    const char* port_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        if (arg == "--port") {
            const char* v = next();
            if (v == nullptr) return 2;
            unsigned p = 0;
            auto [ptr, ec] = std::from_chars(v, v + std::strlen(v), p);
            if (ec != std::errc{} || *ptr != '\0' || p > 65535) return 2;
            port = static_cast<std::uint16_t>(p);
        } else if (arg == "--port-file") {
            port_file = next();
            if (port_file == nullptr) return 2;
        } else {
            std::fprintf(stderr, "usage: %s [--port P] [--port-file F]\n", argv[0]);
            return 2;
        }
    }

    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa {};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    try {
        int listener = listen_tcp(port, port_file);
        Server server;
        if (!server.init()) return 1;
        return server.run(listener);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "kvd_uring: %s\n", e.what());
        return 1;
    }
}
