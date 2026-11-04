// mode: run
// expect: 校验: 通过
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>

// 有界队列：生产快、消费慢时挡住生产者，而不是无限吃内存。
template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t cap) : cap_(cap) {}

    bool push(T v) {
        std::unique_lock lock(m_);
        not_full_.wait(lock, [&] { return closed_ || q_.size() < cap_; });
        if (closed_) return false;
        q_.push_back(std::move(v));
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock lock(m_);
        not_empty_.wait(lock, [&] { return closed_ || !q_.empty(); });
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop_front();
        not_full_.notify_one();
        return v;
    }

    void close() {
        std::lock_guard lock(m_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable not_empty_, not_full_;
    std::deque<T> q_;
    size_t cap_;
    bool closed_ = false;
};

struct Request { int id; uint64_t value; };
struct Stats { int read = 0; int parsed = 0; int summed = 0; uint64_t total = 0; };

int main() {
    constexpr int kItems = 12;
    BoundedQueue<Request> raw(4), parsed(4);
    Stats st;

    // I/O 线程只做“读”，慢在网络和磁盘，不占用计算线程。
    std::thread io([&] {
        for (int i = 0; i < kItems; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (!raw.push(Request{i, uint64_t(i) * 2})) break;
            ++st.read;
        }
        raw.close();
    });

    // 解析线程只做格式转换。
    std::thread parser([&] {
        while (auto r = raw.pop()) {
            Request out{r->id, r->value * 3};
            if (!parsed.push(out)) break;
            ++st.parsed;
        }
        parsed.close();
    });

    // 汇总线程只做累加；主线程只负责启动和收尾。
    std::thread summer([&] {
        while (auto r = parsed.pop()) {
            st.total += r->value;
            ++st.summed;
        }
    });

    io.join();
    parser.join();
    summer.join();

    uint64_t want = 0;
    for (int i = 0; i < kItems; ++i) want += uint64_t(i) * 2 * 3;
    std::printf("I/O 送出 %d 项，解析 %d 项，汇总 %d 项\n", st.read, st.parsed, st.summed);
    std::printf("合计 %llu\n", (unsigned long long)st.total);
    std::printf("校验: %s\n", st.total == want && st.read == kItems ? "通过" : "失败");
}
