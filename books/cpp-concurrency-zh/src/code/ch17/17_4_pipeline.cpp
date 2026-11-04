// mode: run
// output: varies
// expect: 校验: 通过
// tsan: no（计时示例，TSan 下太慢）
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>

using Clock = std::chrono::steady_clock;
using Us = std::chrono::microseconds;

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
    size_t size() {
        std::lock_guard lock(m_);
        return q_.size();
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

struct Item { int id; uint64_t value; };

// 用忙等模拟一段固定耗时的工作，比 sleep_for 的精度高。
void work_for(Us us) {
    auto end = Clock::now() + us;
    while (Clock::now() < end) {
    }
}

int main() {
    constexpr int N = 1000;
    constexpr Us kRead{20}, kParse{20}, kCompute{10}, kWrite{10};
    BoundedQueue<Item> q1(8), q2(8), q3(8);

    auto start = Clock::now();
    std::thread reader([&] {
        for (int i = 0; i < N; ++i) {
            work_for(kRead);
            if (!q1.push(Item{i, uint64_t(i) * 7 + 1})) break;
        }
        q1.close();
    });
    size_t q2_max = 0;
    std::thread parser([&] {
        while (auto it = q1.pop()) {
            work_for(kParse);
            it->value = it->value * 3 + 1;
            if (!q2.push(*it)) break;
            q2_max = std::max(q2_max, q2.size());
        }
        q2.close();
    });
    std::thread computer([&] {
        while (auto it = q2.pop()) {
            work_for(kCompute);
            it->value = it->value ^ (it->value >> 17);
            if (!q3.push(*it)) break;
        }
        q3.close();
    });

    Clock::time_point first{}, at100{}, last{};
    uint64_t checksum = 0;
    int written = 0;
    std::thread writer([&] {
        while (auto it = q3.pop()) {
            work_for(kWrite);
            auto now = Clock::now();
            if (it->id == 0) first = now;
            if (it->id == 100) at100 = now;
            last = now;
            checksum += it->value;
            ++written;
        }
    });

    reader.join();
    parser.join();
    computer.join();
    writer.join();

    double first_us = std::chrono::duration<double, std::micro>(first - start).count();
    double steady = (N - 101) / std::chrono::duration<double>(last - at100).count();
    std::printf("处理 %d 项：读 %lldus / 解析 %lldus / 计算 %lldus / 写 %lldus\n", N,
                (long long)kRead.count(), (long long)kParse.count(),
                (long long)kCompute.count(), (long long)kWrite.count());
    std::printf("首件延迟: %.1f us\n", first_us);
    std::printf("稳态吞吐: %.0f 项/秒\n", steady);
    std::printf("q2 最大长度: %zu / 8\n", q2_max);
    std::printf("校验: %s（%d 项，合计 %llu）\n",
                written == N ? "通过" : "失败", written, (unsigned long long)checksum);
}
