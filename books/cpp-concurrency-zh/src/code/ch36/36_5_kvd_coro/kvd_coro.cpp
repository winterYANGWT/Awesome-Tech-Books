// kvd_coro：用 Boost.Asio 的 C++20 协程写同一个键值服务器。
//
// 每个连接一个协程：co_await 读 -> 解析 -> co_await 写，代码是顺序的。
// 每个连接绑一个 strand；即使 io_context 跑多个线程，同一连接的回调也不会并发。
// 关闭时对每个连接发出 terminal 取消，协程在 co_await 处收到 operation_aborted。
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <csignal>
#include <cstdint>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

// GCC 15 对 Asio 协程帧的 aligned_alloc/operator delete 报了误报的
// -Wmismatched-new-delete；这是库内部成对的分配和释放。
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

namespace asio = boost::asio;
using asio::ip::tcp;

class Server;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Server& server, Store& store)
        : socket_(std::move(socket)), server_(server), store_(store) {}

    void start() {
        auto self = shared_from_this();
        // 外层协程只负责持有 self；真正的工作在 serve() 里。
        asio::co_spawn(socket_.get_executor(),
                       [self]() -> asio::awaitable<void> { co_await self->serve(); },
                       asio::bind_cancellation_slot(signal_.slot(), asio::detached));
    }

    void cancel() { signal_.emit(asio::cancellation_type::terminal); }

private:
    asio::awaitable<void> serve();

    tcp::socket socket_;
    Server& server_;
    Store& store_;
    asio::cancellation_signal signal_;
    Parser parser_;
    std::string out_;
};

class Server {
public:
    explicit Server(Store& store) : store_(store) {}

    int run(std::uint16_t port, const char* port_file) {
        int listener = listen_tcp(port, port_file);
        set_nonblocking(listener);
        acceptor_.assign(tcp::v4(), listener);

        signals_.async_wait([this](const boost::system::error_code& ec, int) {
            if (!ec) begin_stop();
        });
        asio::co_spawn(ioc_, accept_loop(), asio::detached);
        ioc_.run();
        std::printf("accepted=%d sessions_at_exit=%zu\n", accepted_, sessions_.size());
        return sessions_.empty() ? 0 : 1;
    }

    void remove(const std::shared_ptr<Session>& session) {
        sessions_.erase(session);
        if (stopping_ && sessions_.empty()) ioc_.stop();
    }

private:
    // ANCHOR: shutdown
    void begin_stop() {
        if (stopping_) return;
        stopping_ = true;
        boost::system::error_code ec;
        acceptor_.close(ec);
        std::vector<std::shared_ptr<Session>> copy(sessions_.begin(), sessions_.end());
        for (auto& session : copy) session->cancel();
        if (sessions_.empty()) ioc_.stop();
    }

    // ANCHOR_END: shutdown

    asio::awaitable<void> accept_loop() {
        for (;;) {
            try {
                tcp::socket socket = co_await acceptor_.async_accept(
                    asio::make_strand(ioc_), asio::use_awaitable);
                socket.set_option(tcp::no_delay(true));
                ++accepted_;
                auto session = std::make_shared<Session>(std::move(socket), *this, store_);
                sessions_.insert(session);
                session->start();
            } catch (const boost::system::system_error& e) {
                if (e.code() != asio::error::operation_aborted)
                    std::fprintf(stderr, "accept: %s\n", e.what());
                break;
            }
        }
        if (stopping_ && sessions_.empty()) ioc_.stop();
    }

    Store& store_;
    asio::io_context ioc_;
    tcp::acceptor acceptor_{ioc_};
    asio::signal_set signals_{ioc_, SIGTERM, SIGINT};
    std::set<std::shared_ptr<Session>> sessions_;
    bool stopping_ = false;
    int accepted_ = 0;
};

// ANCHOR: session
asio::awaitable<void> Session::serve() {
        char buf[4096];
        try {
            for (;;) {
                std::size_t n = co_await socket_.async_read_some(
                    asio::buffer(buf, sizeof buf), asio::use_awaitable);
                parser_.feed(std::string_view(buf, n));
                while (auto result = parser_.next()) {
                    if (auto* request = std::get_if<Request>(&*result)) {
                        out_ += format_response(store_.apply(*request));
                    } else {
                        out_ += format_response(std::get<ParseError>(*result));
                    }
                }
                if (!out_.empty()) {
                    co_await asio::async_write(socket_, asio::buffer(out_),
                                               asio::use_awaitable);
                    out_.clear();
                }
            }
        } catch (const boost::system::system_error& e) {
            if (e.code() != asio::error::operation_aborted &&
                e.code() != asio::error::eof &&
                e.code() != asio::error::connection_reset) {
                std::fprintf(stderr, "session: %s\n", e.what());
            }
        }
        server_.remove(shared_from_this());
}
// ANCHOR_END: session

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

    std::signal(SIGPIPE, SIG_IGN);
    try {
        Store store;
        Server server(store);
        return server.run(port, port_file);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "kvd_coro: %s\n", e.what());
        return 1;
    }
}
