// mode: run
// 两个 actor 用有界邮箱来回传一个令牌：发送最多领先一条，处理严格串行。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

template <class T>
class Mailbox {
public:
    explicit Mailbox(std::size_t capacity) : capacity_(capacity) {}

    bool send(T value) {
        std::unique_lock<std::mutex> lock(m_);
        not_full_.wait(lock, [&] { return q_.size() < capacity_ || closed_; });
        if (closed_) {
            return false;
        }
        q_.push_back(std::move(value));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> recv() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        T value = std::move(q_.front());
        q_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> q_;
    std::size_t capacity_;
    bool closed_ = false;
};

class Actor {
public:
    Actor(Mailbox<int>& inbox, Mailbox<int>& outbox, int rounds, std::atomic<int>& final)
        : inbox_(inbox), outbox_(outbox), rounds_(rounds), final_(final), thread_([this] { run(); }) {}

    ~Actor() { join(); }

    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    void run() {
        while (auto token = inbox_.recv()) {
            if (*token < rounds_) {
                outbox_.send(*token + 1);
            } else {
                final_.store(*token);
            }
        }
    }

    Mailbox<int>& inbox_;
    Mailbox<int>& outbox_;
    int rounds_;
    std::atomic<int>& final_;
    std::thread thread_;
};

int main() {
    constexpr int kRounds = 1000;
    Mailbox<int> mb_a(4);
    Mailbox<int> mb_b(4);
    std::atomic<int> final{0};
    Actor a(mb_a, mb_b, kRounds, final);
    Actor b(mb_b, mb_a, kRounds, final);

    mb_a.send(0);
    while (final.load() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    mb_a.close();
    mb_b.close();
    a.join();
    b.join();

    std::printf("往返 %d 轮，最终计数 %d\n", kRounds, final.load());
}
