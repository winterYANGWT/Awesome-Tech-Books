// mode: run
// expect: 消费者：处理了 50 个任务
// expect: 队列剩余: 0
// expect: 退出原因: stop_token 已请求: 是

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <iostream>
#include <latch>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

class TaskQueue {
 public:
  void push(int value) {
    {
      std::lock_guard lock(mutex_);
      queue_.push_back(value);
    }
    cv_.notify_one();
  }

  // 返回 nullopt 表示“停止请求已到，队列也空了”
  std::optional<int> wait_pop(std::stop_token token) {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, token, [&] {
      return !queue_.empty() || token.stop_requested();
    });
    if (queue_.empty()) {
      return std::nullopt;
    }
    int value = queue_.front();
    queue_.pop_front();
    return value;
  }

  std::size_t size() const {
    std::lock_guard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable_any cv_;
  std::deque<int> queue_;
};

int main() {
  TaskQueue queue;
  for (int i = 0; i < 50; ++i) {
    queue.push(i);
  }

  std::latch drained(1);
  std::jthread consumer([&](std::stop_token token) {
    int count = 0;
    while (auto value = queue.wait_pop(token)) {
      (void)value;
      ++count;
      if (count == 50) {
        drained.count_down();
      }
    }
    std::cout << "消费者：处理了 " << count << " 个任务\n";
  });

  drained.wait();          // 50 个任务都已取走，消费者回到等待
  consumer.request_stop(); // 停止请求会叫醒 condition_variable_any
  consumer.join();

  std::cout << "队列剩余: " << queue.size() << "\n";
  std::cout << "退出原因: stop_token 已请求: "
            << (consumer.get_stop_token().stop_requested() ? "是" : "否") << "\n";
}
