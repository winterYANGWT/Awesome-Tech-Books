// mode: run
// output: varies
// expect: 主线程：已置停止标志
// expect: 工作线程：在第 \d+ 个检查点看到停止请求
// expect: 清理 1 次

#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
  std::atomic<bool> stop{false};
  std::atomic<long> batches{0};
  std::atomic<long> checkpoints{0};
  std::atomic<long> cleanups{0};

  std::thread worker([&] {
    // 用 RAII 记录“线程退出前清理过”
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
      checkpoints.fetch_add(1, std::memory_order_relaxed);
      if (stop.load(std::memory_order_relaxed)) {
        std::cout << "工作线程：在第 " << b + 1 << " 个检查点看到停止请求\n";
        return;
      }
    }
  });

  // 让工作线程跑几批，再请求停止；具体批数取决于调度
  while (batches.load(std::memory_order_relaxed) < 3) {
    std::this_thread::yield();
  }
  stop.store(true, std::memory_order_relaxed);
  std::cout << "主线程：已置停止标志\n";
  worker.join();
  std::cout << "主线程：join 返回，已处理 " << batches.load() << " 批，"
            << "检查点 " << checkpoints.load() << " 次，清理 "
            << cleanups.load() << " 次\n";
}
