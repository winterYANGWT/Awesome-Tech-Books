// kvd 的 Libevent 版：监听用 evconnlistener，连接用 bufferevent。
// 协议和 Store 原样复用；这一层只负责字节进出和连接生命周期。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace {

constexpr std::size_t kHighWater = 64 * 1024;
constexpr std::size_t kLowWater = 16 * 1024;

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

void write_port_file(int fd, const char *path) {
    sockaddr_in addr{};
    socklen_t len = sizeof addr;
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len) != 0)
        throw std::system_error(errno, std::generic_category(), "getsockname");
    const std::uint16_t actual = ntohs(addr.sin_port);
    const std::string tmp =
        std::string(path) + ".tmp." + std::to_string(static_cast<long>(getpid()));
    int out = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (out < 0) throw std::system_error(errno, std::generic_category(), "open port file");
    const std::string text = std::to_string(actual) + "\n";
    bool ok = write_all(out, text);
    int saved = errno;
    close(out);
    if (!ok) {
        unlink(tmp.c_str());
        throw std::system_error(saved, std::generic_category(), "write port file");
    }
    if (rename(tmp.c_str(), path) != 0) {
        saved = errno;
        unlink(tmp.c_str());
        throw std::system_error(saved, std::generic_category(), "rename port file");
    }
}

struct Server;

struct Connection : std::enable_shared_from_this<Connection> {
    Connection(Server *owner, struct event_base *base, int fd)
        : owner(owner), fd(fd) {
        bev = bufferevent_socket_new(base, fd,
                                     BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        if (bev == nullptr) {
            ::close(fd);
            this->fd = -1;
            return;
        }
// ANCHOR: bev
        bufferevent_setcb(bev, on_read, on_write, on_event, this);
        bufferevent_setwatermark(bev, EV_WRITE, kLowWater, 0);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
// ANCHOR_END: bev
    }

    ~Connection() {
        // BEV_OPT_CLOSE_ON_FREE 会让 bufferevent_free 关掉 fd，不要再 close 一次。
        if (bev != nullptr) bufferevent_free(bev);
    }

    void close();
    void do_read(struct bufferevent *bev);
    void do_write(struct bufferevent *bev);

    static void on_read(struct bufferevent *bev, void *ctx) {
        static_cast<Connection *>(ctx)->do_read(bev);
    }
    static void on_write(struct bufferevent *bev, void *ctx) {
        static_cast<Connection *>(ctx)->do_write(bev);
    }
    static void on_event(struct bufferevent *bev, short what, void *ctx) {
        (void)bev;
        auto self = static_cast<Connection *>(ctx)->shared_from_this();
        if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
            self->close();
    }

    Server *owner;
    struct bufferevent *bev = nullptr;
    int fd = -1;
    Parser parser;
    bool closed = false;
    bool read_paused = false;
};

struct Server {
    explicit Server(Store *store) : store(store) {
        base = event_base_new();
        if (base == nullptr) throw std::runtime_error("event_base_new");
    }

    ~Server() { shutdown(); }

    void listen_on(std::uint16_t port, const char *port_file) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        listener = evconnlistener_new_bind(
            base, on_accept, this,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 128,
            reinterpret_cast<sockaddr *>(&addr), sizeof addr);
        if (listener == nullptr) throw std::runtime_error("evconnlistener_new_bind");
        if (port_file != nullptr) write_port_file(evconnlistener_get_fd(listener), port_file);
    }

    static void on_accept(struct evconnlistener *l, evutil_socket_t fd,
                          struct sockaddr *sa, int len, void *ctx) {
        (void)l;
        (void)sa;
        (void)len;
        static_cast<Server *>(ctx)->accept(fd);
    }

    void accept(evutil_socket_t fd) {
        auto conn = std::make_shared<Connection>(this, base, static_cast<int>(fd));
        if (conn->fd < 0) return;
        conns.emplace(conn.get(), conn);
        ++served;
    }

    void remove(Connection *c) { conns.erase(c); }

    void stop() {
        if (listener != nullptr) {
            evconnlistener_disable(listener);
        }
        event_base_loopexit(base, nullptr);
    }

    void run() {
        struct event *term = evsignal_new(base, SIGTERM, on_signal, this);
        struct event *intr = evsignal_new(base, SIGINT, on_signal, this);
        event_add(term, nullptr);
        event_add(intr, nullptr);
        event_base_dispatch(base);
        event_free(term);
        event_free(intr);
    }

    static void on_signal(evutil_socket_t sig, short what, void *ctx) {
        (void)sig;
        (void)what;
        static_cast<Server *>(ctx)->stop();
    }

    void shutdown() {
        if (base == nullptr) return;
        conns.clear();  // Connection 析构会 bufferevent_free，由它关 fd
        if (listener != nullptr) {
            evconnlistener_free(listener);
            listener = nullptr;
        }
        event_base_free(base);
        base = nullptr;
    }

    struct event_base *base = nullptr;
    struct evconnlistener *listener = nullptr;
    Store *store;
    std::unordered_map<Connection *, std::shared_ptr<Connection>> conns;
    std::uint64_t served = 0;
    std::uint64_t bytes = 0;
};


void Connection::close() {
    if (closed) return;
    closed = true;
    if (bev != nullptr) {
        bufferevent_free(bev);  // BEV_OPT_CLOSE_ON_FREE 会关 fd
        bev = nullptr;
    }
    fd = -1;
    owner->remove(this);
}

void Connection::do_read(struct bufferevent *bev) {
    auto self = shared_from_this();  // 回调里可能被 map 移除，先保住自己
    evbuffer *in = bufferevent_get_input(bev);
    char buf[4096];
    int n = 0;
    while ((n = evbuffer_remove(in, buf, sizeof buf)) > 0) {
        parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
        owner->bytes += static_cast<std::uint64_t>(n);
    }
    std::string out;
    while (auto result = parser.next()) {
        if (auto *req = std::get_if<Request>(&*result))
            out += format_response(owner->store->apply(*req));
        else
            out += format_response(std::get<ParseError>(*result));
    }
// ANCHOR: backpressure
    if (!out.empty()) bufferevent_write(bev, out.data(), out.size());
    if (evbuffer_get_length(bufferevent_get_output(bev)) > kHighWater) {
        bufferevent_disable(bev, EV_READ);  // 输出积压：先别读新请求
        read_paused = true;
    }
// ANCHOR_END: backpressure
}

void Connection::do_write(struct bufferevent *bev) {
    if (read_paused &&
        evbuffer_get_length(bufferevent_get_output(bev)) <= kLowWater) {
        bufferevent_enable(bev, EV_READ);
        read_paused = false;
    }
}


}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F)\n", argv[0]);
        return 2;
    }

    signal(SIGPIPE, SIG_IGN);
    Store store(16);
    try {
        Server server(&store);
        server.listen_on(port, port_file);
        server.run();
        std::printf("server: served=%llu input_bytes=%llu\n",
                    static_cast<unsigned long long>(server.served),
                    static_cast<unsigned long long>(server.bytes));
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
    return 0;
}
