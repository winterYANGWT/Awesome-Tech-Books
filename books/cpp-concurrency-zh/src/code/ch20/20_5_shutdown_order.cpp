// mode: run
// output: varies
// expect: 并行关闭总耗时
// expect: 串行关闭总耗时
// expect: 串行/并行

#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <latch>
#include <stop_token>
#include <thread>
#include <vector>

namespace {

constexpr int kWorkers = 8;
constexpr auto kCleanup = std::chrono::milliseconds(40);

struct WorkerResult {
  long long cleanup_us = 0;
};

void worker(std::stop_token token, WorkerResult* result, std::latch* ready) {
  ready->count_down();
  while (!token.stop_requested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  const auto start = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(kCleanup); /* 模拟释放资源、写日志 */
  const auto end = std::chrono::steady_clock::now();
  result->cleanup_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

double run(bool parallel) {
  std::array<WorkerResult, kWorkers> results{};
  std::latch ready(kWorkers);
  std::vector<std::jthread> threads;
  threads.reserve(kWorkers);

  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < kWorkers; ++i) {
    threads.emplace_back(worker, &results[i], &ready);
  }
  ready.wait();

  if (parallel) {
    for (auto& thread : threads) {
      thread.request_stop(); /* 先全部请求停止 */
    }
    for (auto& thread : threads) {
      thread.join(); /* 再统一等待，清理并行进行 */
    }
  } else {
    for (auto& thread : threads) {
      thread.request_stop(); /* 通知一个 */
      thread.join();         /* 等它清理完再通知下一个 */
    }
  }
  const auto end = std::chrono::steady_clock::now();

  std::cout << (parallel ? "并行" : "串行") << "关闭总耗时: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";
  for (int i = 0; i < kWorkers; ++i) {
    std::cout << "  线程 " << i << " 清理 "
              << results[i].cleanup_us / 1000 << " ms\n";
  }
  return std::chrono::duration<double>(end - start).count();
}

}  // namespace

int main() {
  const double parallel = run(true);
  const double serial = run(false);
  std::cout << "串行/并行: " << serial / parallel << " 倍\n";
}
