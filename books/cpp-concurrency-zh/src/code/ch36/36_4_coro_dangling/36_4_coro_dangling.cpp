// 反例：协程按引用捕获一个局部 std::string，函数返回后定时器才恢复它。
// 用 ASan 的 detect_stack_use_after_return=1 让这块栈变成“已返回”的毒区。
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <chrono>
#include <iostream>
#include <string>

namespace asio = boost::asio;
using namespace std::chrono_literals;

asio::awaitable<void> show_later(std::string& text) {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    timer.expires_after(50ms);
    co_await timer.async_wait(asio::use_awaitable);
    std::cout << "text=" << text << "\n";  // start() 已经返回，text 已销毁
}

__attribute__((noinline)) void start(asio::io_context& ioc) {
    std::string text = "this string dies before the timer fires";
    asio::co_spawn(ioc, show_later(text), asio::detached);
}

int main() {
    asio::io_context ioc;
    start(ioc);
    ioc.run();
}
