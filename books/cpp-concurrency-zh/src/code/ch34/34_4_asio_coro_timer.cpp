// Asio 的 C++20 协程写法：co_spawn 启动一个 awaitable<void>，
// flags: -Wno-tsan
// steady_timer 每次 co_await 恢复协程，顺序就是代码的顺序。
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <chrono>
#include <cstdio>

namespace asio = boost::asio;

asio::awaitable<void> ticker() {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    for (int i = 1; i <= 3; ++i) {
        timer.expires_after(std::chrono::milliseconds(50));
        co_await timer.async_wait(asio::use_awaitable);
        std::printf("tick %d\n", i);
    }
}

int main() {
    asio::io_context io;
    asio::co_spawn(io, ticker(), asio::detached);
    io.run();
    return 0;
}
