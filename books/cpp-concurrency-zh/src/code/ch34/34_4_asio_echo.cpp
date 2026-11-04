// Asio 回调式回射：一个 io_context、一个 acceptor、两个客户端，全部异步。
// flags: -Wno-tsan
// 和 34.2 的 Libevent 版对照代码形状：没有事件对象，只有 async_* + handler。
#include <boost/asio.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using boost::system::error_code;

struct Server;

struct Conn : std::enable_shared_from_this<Conn> {
    Conn(tcp::socket s, Server *owner) : socket(std::move(s)), owner(owner) {}

    void start() { do_read(); }

    void do_read();
    void close();

    tcp::socket socket;
    Server *owner;
    char buf[1024];
};

struct Server {
    Server(asio::io_context &io, std::uint16_t port)
        : ioc(io),
          acceptor(io, tcp::endpoint(asio::ip::make_address("127.0.0.1"), port)),
          deadline(io) {
        deadline.expires_after(std::chrono::seconds(3));
        deadline.async_wait([this](error_code ec) {
            std::printf("deadline: %s\n", ec ? "canceled" : "expired");
            ioc.stop();
        });
        do_accept();
    }

    void do_accept() {
        acceptor.async_accept([this](error_code ec, tcp::socket s) {
            if (!ec) {
                auto c = std::make_shared<Conn>(std::move(s), this);
                conns.emplace(c.get(), c);
                ++served;
                c->start();
                do_accept();
            }
        });
    }

    void remove(Conn *c) { conns.erase(c); }

    void stop_accepting() {
        error_code ec;
        acceptor.close(ec);
    }

    asio::io_context &ioc;
    tcp::acceptor acceptor;
    asio::steady_timer deadline;
    std::unordered_map<Conn *, std::shared_ptr<Conn>> conns;
    std::uint64_t served = 0;
    std::uint64_t echoed = 0;
};

void Conn::do_read() {
    auto self = shared_from_this();
    socket.async_read_some(asio::buffer(buf), [self](error_code ec, std::size_t n) {
        if (ec) {
            self->close();
            return;
        }
        self->owner->echoed += n;
        asio::async_write(self->socket, asio::buffer(self->buf, n),
                          [self](error_code ec2, std::size_t) {
                              if (ec2) self->close();
                              else self->do_read();
                          });
    });
}

void Conn::close() {
    error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    socket.close(ec);
    owner->remove(this);
}

static void start_client(asio::io_context &io, Server &server, std::uint16_t port,
                         int id, const std::string &payload, int &done,
                         std::size_t &received) {
    auto sock = std::make_shared<tcp::socket>(io);
    tcp::endpoint ep(asio::ip::make_address("127.0.0.1"), port);
    sock->async_connect(ep, [&, sock, id, payload](error_code ec) {
        if (ec) {
            std::printf("client %d: connect failed\n", id);
            return;
        }
        asio::async_write(*sock, asio::buffer(payload),
                          [&, sock, id, payload](error_code ec2, std::size_t) {
                              if (ec2) {
                                  std::printf("client %d: write failed\n", id);
                                  return;
                              }
                              auto got = std::make_shared<std::string>(payload.size(), '\0');
                              asio::async_read(
                                  *sock, asio::buffer(*got),
                                  asio::transfer_exactly(payload.size()),
                                  [&, sock, id, payload, got](error_code ec3, std::size_t n) {
                                      if (!ec3 && *got == payload) {
                                          ++done;
                                          received += n;
                                          if (done == 2) {
                                              server.stop_accepting();
                                              server.deadline.cancel();
                                          }
                                      } else {
                                          std::printf("client %d: echo mismatch\n", id);
                                      }
                                      error_code ignore;
                                      sock->close(ignore);
                                  });
                          });
    });
}

int main() {
    asio::io_context io;
    Server server(io, 0);
    const std::uint16_t port = server.acceptor.local_endpoint().port();

    int done = 0;
    std::size_t received = 0;
    start_client(io, server, port, 0, "hello-one", done, received);
    start_client(io, server, port, 1, "hello-two", done, received);

    io.run();
    std::printf("clients: done=%d received=%zu\n", done, received);
    std::printf("server: served=%llu echoed=%llu\n",
                static_cast<unsigned long long>(server.served),
                static_cast<unsigned long long>(server.echoed));
    return 0;
}
