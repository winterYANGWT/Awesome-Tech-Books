// mode: run
// output: varies
// expect: request_stop 第一次: 1，第二次: 0
// expect: join 后 stop_requested: 1
// expect: 清理 1 次

#include <atomic>
#include <cstdint>
#include <iostream>
#include <stop_token>
#include <thread>

int main() {
  std::atomic<long> batches{0};
  std::atomic<long> cleanups{0};

  std::jthread worker([&](std::stop_token token) {
    struct Cleanup {
      std::atomic<long>& n;
      ~Cleanup() { n.fetch_add(1, std::memory_order_relaxed); }
    } guard{cleanups};

    constexpr long kBatch = 200000;
    for (long b = 0; b < 1000; ++b) {
      volatile std::uint64_t sum = 0;
      for (long i = 0; i < kBatch; ++i) {
        sum = sum + static_cast<std::uint64_t>(i);
      }
      batches.store(b + 1, std::memory_order_relaxed);
      if (token.stop_requested()) {
        std::cout << "工作线程：在第 " << b + 1 << " 个检查点退出\n";
        return;
      }
    }
  });

  while (batches.load(std::memory_order_relaxed) < 3) {
    std::this_thread::yield();
  }
  bool first = worker.request_stop();
  bool second = worker.request_stop();
  worker.join();

  std::cout << "request_stop 第一次: " << first
            << "，第二次: " << second << "\n";
  std::cout << "join 后 stop_requested: "
            << worker.get_stop_token().stop_requested() << "\n";
  std::cout << "主线程：清理 " << cleanups.load() << " 次，"
            << "已处理 " << batches.load() << " 批\n";
}
