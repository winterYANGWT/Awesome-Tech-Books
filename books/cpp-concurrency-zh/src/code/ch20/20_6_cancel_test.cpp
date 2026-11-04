// mode: run
// expect: 请求早于启动
// expect: 请求发生在等待中
// expect: 请求晚于完成
// expect: 重复请求
// expect: 请求后立即析构

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <iostream>
#include <latch>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

namespace {

struct Cleanup {
  std::atomic<int>* count;
  ~Cleanup() { count->fetch_add(1, std::memory_order_relaxed); }
};

struct Result {
  std::string checkpoint;
  int cleanup = 0;
  int remaining = 0;
};

void print_result(int number, const char* name, const Result& result) {
  std::cout << number << " " << name << "：检查点=" << result.checkpoint
            << "，清理=" << result.cleanup
            << "，剩余任务=" << result.remaining << "\n";
}

Result early_request() {
  std::stop_source source;
  source.request_stop(); /* 线程还没创建，请求已经发出 */
  std::atomic<int> cleanup{0};
  std::string checkpoint = "未运行";

  {
    std::jthread thread([&] {
      Cleanup guard{&cleanup};
      checkpoint = source.get_token().stop_requested() ? "启动前已停止" : "循环正常";
    });
  }
  return {checkpoint, cleanup.load(), 0};
}

Result request_while_waiting() {
  std::mutex mutex;
  std::condition_variable_any cv;
  std::deque<int> queue;
  std::latch drained(1);
  std::latch waiting(1);
  std::atomic<int> cleanup{0};
  std::string checkpoint = "未唤醒";

  for (int i = 0; i < 20; ++i) {
    queue.push_back(i);
  }

  std::jthread consumer([&](std::stop_token token) {
    Cleanup guard{&cleanup};
    int count = 0;
    while (true) {
      std::unique_lock lock(mutex);
      if (count == 20 && queue.empty()) {
        waiting.count_down(); /* 已经取完 20 个，马上进入等待 */
      }
      cv.wait(lock, token, [&] {
        return !queue.empty() || token.stop_requested();
      });
      if (queue.empty()) {
        break;
      }
      queue.pop_front();
      ++count;
      if (count == 20) {
        drained.count_down();
      }
    }
    checkpoint = token.stop_requested() ? "等待中被唤醒" : "正常退出";
  });

  drained.wait();
  waiting.wait(); /* 消费者已经取完 20 个任务 */
  {
    /* 拿到消费者在 cv.wait 里释放的互斥量，确认它已经进入等待 */
    std::unique_lock lock(mutex);
  }
  consumer.request_stop();
  consumer.join();

  std::size_t remaining = 0;
  {
    std::lock_guard lock(mutex);
    remaining = queue.size();
  }
  return {checkpoint, cleanup.load(), static_cast<int>(remaining)};
}

Result request_after_done() {
  std::atomic<int> cleanup{0};
  std::string checkpoint = "未完成";
  std::jthread thread([&] {
    Cleanup guard{&cleanup};
    checkpoint = "完成后才请求";
  });
  thread.join();
  const bool requested = thread.request_stop();
  checkpoint += requested ? "（request_stop=true）" : "（request_stop=false）";
  return {checkpoint, cleanup.load(), 0};
}

Result duplicate_request() {
  std::latch started(1);
  std::atomic<int> cleanup{0};
  std::string checkpoint = "未运行";
  std::jthread thread([&](std::stop_token token) {
    Cleanup guard{&cleanup};
    started.count_down();
    while (!token.stop_requested()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    checkpoint = "第一次请求后退出";
  });
  started.wait();
  const bool first = thread.request_stop();
  const bool second = thread.request_stop();
  thread.join();
  checkpoint += first && !second ? "（第二次返回 false）" : "（返回值不符）";
  return {checkpoint, cleanup.load(), 0};
}

Result request_then_destroy() {
  std::latch started(1);
  std::atomic<int> cleanup{0};
  std::string checkpoint = "未运行";
  {
    std::jthread thread([&](std::stop_token token) {
      Cleanup guard{&cleanup};
      started.count_down();
      while (!token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      checkpoint = "析构时汇入";
    });
    started.wait();
    thread.request_stop();
  }
  return {checkpoint, cleanup.load(), 0};
}

}  // namespace

int main() {
  print_result(1, "请求早于启动", early_request());
  print_result(2, "请求发生在等待中", request_while_waiting());
  print_result(3, "请求晚于完成", request_after_done());
  print_result(4, "重复请求", duplicate_request());
  print_result(5, "请求后立即析构", request_then_destroy());
}
