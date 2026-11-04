// mode: run
// expect: 回调 A：在调用 request_stop 的线程上执行: 是
// expect: 回调 B：在调用 request_stop 的线程上执行: 是
// expect: request_stop 返回: 1
// expect: 停止后注册的回调 C：构造返回前已执行: 是

#include <atomic>
#include <iostream>
#include <stop_token>
#include <thread>

int main() {
  std::stop_source source;
  std::stop_token token = source.get_token();
  const std::thread::id caller = std::this_thread::get_id();

  // 回调只做记录，不打印：避免多个回调的执行顺序影响输出
  std::atomic<bool> a_on_caller{false};
  std::atomic<bool> b_on_caller{false};
  std::stop_callback callback_a(token, [&] {
    a_on_caller.store(std::this_thread::get_id() == caller,
                      std::memory_order_relaxed);
  });
  std::stop_callback callback_b(token, [&] {
    b_on_caller.store(std::this_thread::get_id() == caller,
                      std::memory_order_relaxed);
  });

  std::cout << "主线程：调用 request_stop\n";
  bool requested = source.request_stop();
  std::cout << "request_stop 返回: " << requested << "\n";
  std::cout << "回调 A：在调用 request_stop 的线程上执行: "
            << (a_on_caller.load() ? "是" : "否") << "\n";
  std::cout << "回调 B：在调用 request_stop 的线程上执行: "
            << (b_on_caller.load() ? "是" : "否") << "\n";

  // 注册时停止已经请求：回调在构造函数返回前、构造线程上执行
  std::atomic<bool> c_ran{false};
  std::stop_callback callback_c(token, [&] {
    c_ran.store(std::this_thread::get_id() == caller,
                std::memory_order_relaxed);
  });
  std::cout << "停止后注册的回调 C：构造返回前已执行: "
            << (c_ran.load() ? "是" : "否") << "\n";
}
