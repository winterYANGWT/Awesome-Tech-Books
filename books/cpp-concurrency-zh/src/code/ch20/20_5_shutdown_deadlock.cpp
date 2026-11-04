// mode: hang
// expect: 仍未结束

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <latch>
#include <mutex>
#include <stop_token>
#include <thread>

std::mutex mutex_a;
std::mutex mutex_b;
std::mutex wait_mutex;
std::condition_variable_any wait_cv;
std::latch ready(2);

void worker_a(std::stop_token token) {
  std::unique_lock lock_a(mutex_a);
  ready.count_down();
  while (!token.stop_requested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  lock_a.unlock();

  std::cout << "线程 A：清理时尝试锁 mutex_b\n";
  std::lock_guard lock_b(mutex_b); /* B 一直持有 mutex_b */
  std::cout << "线程 A：拿到 mutex_b（不会发生）\n";
}

void worker_b(std::stop_token token) {
  std::lock_guard lock_b(mutex_b); /* 持有 mutex_b，等自己的停止请求 */
  ready.count_down();
  std::unique_lock wait_lock(wait_mutex);
  wait_cv.wait(wait_lock, token, [&] { return token.stop_requested(); });
  std::cout << "线程 B：收到停止请求（不会发生）\n";
}

int main() {
  std::jthread a(worker_a);
  std::jthread b(worker_b);
  ready.wait();

  std::cout << "主线程：只对 A 请求停止，然后 join A\n";
  a.request_stop();
  a.join(); /* A 等 mutex_b；B 等自己的停止请求；主线程等 A */
  std::cout << "主线程：A 已退出（不会发生）\n";
}
