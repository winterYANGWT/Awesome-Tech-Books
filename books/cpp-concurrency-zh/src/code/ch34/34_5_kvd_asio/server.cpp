// kvd 的 Asio 回调式版本：每个连接一个 strand，io_context 跑多个线程。
// 协议和 Store 原样复用；连接状态只在它自己的 strand 里改。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <boost/asio.hpp>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using boost::system::error_code;

namespace {

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

void write_port_file(std::uint16_t port, const char *path) {
    const std::string tmp =
        std::string(path) + ".tmp." + std::to_string(static_cast<long>(getpid()));
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) throw std::runtime_error("open port file");
        out << port << "\n";
        out.close();
        if (!out) throw std::runtime_error("write port file");
    }
    if (std::rename(tmp.c_str(), path) != 0) {
        ::unlink(tmp.c_str());
        throw std::runtime_error("rename port file");
    }
}

struct Server;

struct Connection : std::enable_shared_from_this<Connection> {
    Connection(tcp::socket s, Server *owner) : socket(std::move(s)), owner(owner) {}

    ~Connection() {
        error_code ec;
        socket.close(ec);
    }

    void start() { read_next(); }
    void read_next();
    void do_write();
    void close();

    tcp::socket socket;
    Server *owner;
    asio::streambuf input;
    std::string output;
    Parser parser;
    bool closed = false;
};

struct Server {
    Server(asio::io_context &io, std::uint16_t port, const char *port_file, Store *store)
        : ioc(io), acceptor(io), signals(io, SIGTERM, SIGINT), store(store) {
        acceptor.open(tcp::v4());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.bind(tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
        acceptor.listen(128);
        const std::uint16_t actual = acceptor.local_endpoint().port();
        if (port_file != nullptr) write_port_file(actual, port_file);

        signals.async_wait([this](error_code ec, int) {
            // 只停 io_context；acceptor 等 run() 返回、线程 join 后由析构关闭。
            // 在信号完成所在的任意线程里直接 close acceptor，会和 accept 的发起竞争。
            if (!ec) ioc.stop();
        });
        do_accept();
    }

    void do_accept() {
// ANCHOR: strand
        acceptor.async_accept(asio::make_strand(ioc),
                              [this](error_code ec, tcp::socket s) {
                                  if (ec) return;
                                  auto c = std::make_shared<Connection>(std::move(s), this);
// ANCHOR_END: strand
                                  {
                                      std::lock_guard<std::mutex> lock(map_mutex);
                                      conns.emplace(c.get(), c);
                                  }
                                  served.fetch_add(1, std::memory_order_relaxed);
                                  c->start();
                                  do_accept();
                              });
    }

    void remove(Connection *c) {
        std::lock_guard<std::mutex> lock(map_mutex);
        conns.erase(c);
    }

    void note_bytes(std::size_t n) {
        bytes.fetch_add(n, std::memory_order_relaxed);
    }

    void run(int threads) {
        std::vector<std::thread> pool;
        for (int i = 1; i < threads; ++i)
            pool.emplace_back([this] { ioc.run(); });
        ioc.run();
        for (auto &t : pool) t.join();
    }

    asio::io_context &ioc;
    tcp::acceptor acceptor;
    asio::signal_set signals;
    Store *store;
    std::mutex map_mutex;
    std::unordered_map<Connection *, std::shared_ptr<Connection>> conns;
    std::atomic<std::uint64_t> served{0};
    std::atomic<std::uint64_t> bytes{0};
};

// ANCHOR: read
void Connection::read_next() {
    auto self = shared_from_this();
    asio::async_read_until(socket, input, '\n',
                           [self](error_code ec, std::size_t n) {
                               if (ec) {
                                   self->close();
                                   return;
                               }
                               (void)n;
                               std::istream is(&self->input);
                               std::string line;
                               std::getline(is, line);
                               line.push_back('\n');
                               self->parser.feed(line);
                               self->output.clear();
                               while (auto result = self->parser.next()) {
                                   if (auto *req = std::get_if<Request>(&*result)) {
                                       self->output +=
                                           format_response(self->owner->store->apply(*req));
                                   } else {
                                       self->output +=
                                           format_response(std::get<ParseError>(*result));
                                   }
                               }
                               self->owner->note_bytes(line.size());
                               self->do_write();
                           });
}
// ANCHOR_END: read

void Connection::do_write() {
    if (output.empty()) {
        read_next();
        return;
    }
    auto self = shared_from_this();
    asio::async_write(socket, asio::buffer(output),
                      [self](error_code ec, std::size_t) {
                          if (ec) {
                              self->close();
                              return;
                          }
                          self->output.clear();
                          self->read_next();
                      });
}

void Connection::close() {
    if (closed) return;
    closed = true;
    error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    socket.close(ec);
    owner->remove(this);
}

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    int threads = 2;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            threads = std::atoi(argv[++i]);
            if (threads < 1 || threads > 32) {
                std::fprintf(stderr, "--threads must be 1..32\n");
                return 2;
            }
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F) [--threads N]\n", argv[0]);
        return 2;
    }

    signal(SIGPIPE, SIG_IGN);
    Store store(16);
    try {
        asio::io_context io;
        Server server(io, port, port_file, &store);
        server.served.store(0);
        server.run(threads);
        std::printf("server: served=%llu input_bytes=%llu\n",
                    static_cast<unsigned long long>(server.served.load()),
                    static_cast<unsigned long long>(server.bytes.load()));
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
    return 0;
}
