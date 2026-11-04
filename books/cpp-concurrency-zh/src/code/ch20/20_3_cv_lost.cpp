// mode: hang
// expect: 仍未结束

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>

int main() {
  std::mutex mutex;
  std::condition_variable cv;
  std::deque<int> queue;
  std::atomic<bool> stop{false};
  std::latch checked(1);
  std::latch release(1);

  std::thread consumer([&] {
    while (!stop.load()) {
      std::unique_lock lock(mutex);
      std::cout << "消费者：已经检查过 stop=false，准备进入 wait\n";
      checked.count_down();
      release.wait(); /* 强制交错：主线程在这里置位 stop */
      cv.wait(lock, [&] { return !queue.empty(); });
      if (!queue.empty()) {
        queue.pop_front();
      }
    }
    std::cout << "消费者：退出循环（不会发生）\n";
  });

  checked.wait();
  std::cout << "主线程：置 stop=true，但不调用 notify_all\n";
  stop.store(true);
  release.count_down();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "主线程：join 等待消费者\n";
  consumer.join();
  return 0;
}
