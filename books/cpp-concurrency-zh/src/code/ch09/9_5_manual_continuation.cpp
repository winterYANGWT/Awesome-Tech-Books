// mode: run
// expect: 三步都在同一个后台线程: 是
// expect: 登录结果: alice 的 3 个订单，共 128 元
// expect: 异常: 订单服务超时
// expect: 异常前完成的步骤: 1
// 没有 then：用回调链把三步串起来，异常沿回调往下传。
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace {

// 一个很小的后台执行器：回调都在这一个线程上按提交顺序执行。
class Worker {
public:
    Worker() : thread_([this] { loop(); }) {}

    ~Worker() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        thread_.join();
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    void post(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(job));
        }
        cv_.notify_one();
    }

private:
    void loop() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
                if (stop_ && queue_.empty()) {
                    return;
                }
                job = std::move(queue_.front());
                queue_.pop_front();
            }
            job();
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> queue_;
    bool stop_ = false;
    std::thread thread_;
};

struct Outcome {
    std::string value;
    std::exception_ptr error;
};

using Next = std::function<void(Outcome)>;
using Step = std::function<std::string()>;

// 一步做完后调用 next；抛出的异常装进 Outcome，不在这里处理。
void post_step(Worker& worker, Step step, Next next) {
    worker.post([step = std::move(step), next = std::move(next)] {
        Outcome out;
        try {
            out.value = step();
        } catch (...) {
            out.error = std::current_exception();
        }
        next(std::move(out));
    });
}

void fetch_user(Worker& worker, Next next) {
    post_step(worker, [] { return std::string("alice"); }, std::move(next));
}

void fetch_orders(Worker& worker, std::string user, bool fail, Next next) {
    post_step(worker,
              [user = std::move(user), fail]() -> std::string {
                  if (fail) {
                      throw std::runtime_error("订单服务超时");
                  }
                  return user == "alice" ? "3" : "0";
              },
              std::move(next));
}

void summarize(Worker& worker, std::string user, std::string orders, Next next) {
    post_step(worker,
              [user = std::move(user), orders = std::move(orders)] {
                  return user + " 的 " + orders + " 个订单，共 128 元";
              },
              std::move(next));
}

struct LoginResult {
    std::string text;
    std::exception_ptr error;
    int steps = 0;
    bool all_background = true;
};

LoginResult run_login(Worker& worker, std::thread::id main_id, bool fail_orders) {
    auto promise = std::make_shared<std::promise<Outcome>>();
    std::future<Outcome> future = promise->get_future();
    auto steps = std::make_shared<std::atomic<int>>(0);
    auto background = std::make_shared<std::atomic<bool>>(true);

    auto record = [steps, background, main_id] {
        steps->fetch_add(1, std::memory_order_relaxed);
        if (std::this_thread::get_id() == main_id) {
            background->store(false, std::memory_order_relaxed);
        }
    };

    fetch_user(worker, [&, promise, record](Outcome user) {
        if (user.error) {
            promise->set_value(Outcome{"", user.error});
            return;
        }
        record();
        const std::string user_name = user.value;
        fetch_orders(worker, user_name, fail_orders,
                     [&, promise, record, user_name](Outcome orders) {
                         if (orders.error) {
                             promise->set_value(Outcome{"", orders.error});
                             return;
                         }
                         record();
                         summarize(worker, user_name, orders.value,
                                   [&, promise, record](Outcome summary) {
                                       record();
                                       promise->set_value(std::move(summary));
                                   });
                     });
    });

    Outcome out = future.get();
    return LoginResult{out.value, out.error, steps->load(),
                       background->load()};
}

std::string describe(std::exception_ptr error) {
    try {
        if (error) {
            std::rethrow_exception(error);
        }
    } catch (const std::exception& e) {
        return e.what();
    } catch (...) {
        return "未知异常";
    }
    return "无异常";
}

}  // namespace

int main() {
    Worker worker;
    const std::thread::id main_id = std::this_thread::get_id();

    const LoginResult ok = run_login(worker, main_id, false);
    std::printf("三步都在同一个后台线程: %s\n", ok.all_background ? "是" : "否");
    std::printf("登录结果: %s\n", ok.text.c_str());

    const LoginResult bad = run_login(worker, main_id, true);
    std::printf("异常: %s\n", describe(bad.error).c_str());
    std::printf("异常前完成的步骤: %d\n", bad.steps);
}
