// mode: run
// expect: 15 个格子：15 通过，0 失败
// 测试矩阵：3 种队列状态 × 5 种操作组合。多线程的格子用 latch 同时起跑，
// 每个格子都做确定断言，不靠 sleep 猜时序。
#include <cstdio>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace {

constexpr std::size_t kCapacity = 2;

// ANCHOR: queue
class BoundedQueue {
public:
    bool try_push(int value) {
        std::lock_guard<std::mutex> lock(m_);
        if (q_.size() >= kCapacity) {
            return false;
        }
        q_.push(value);
        return true;
    }

    bool try_pop(int& out) {
        std::lock_guard<std::mutex> lock(m_);
        if (q_.empty()) {
            return false;
        }
        out = q_.front();
        q_.pop();
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
    }

private:
    mutable std::mutex m_;
    std::queue<int> q_;
};
// ANCHOR_END: queue

void prefill(BoundedQueue& q, int state) {
    if (state >= 1) {
        (void)q.try_push(0);
    }
    if (state >= 2) {
        (void)q.try_push(1);
    }
}

int multi_push(BoundedQueue& q) {
    std::latch ready{2};
    std::latch go{1};
    bool ok0 = false;
    bool ok1 = false;

    std::thread t0([&] {
        ready.count_down();
        go.wait();
        ok0 = q.try_push(10);
    });
    std::thread t1([&] {
        ready.count_down();
        go.wait();
        ok1 = q.try_push(11);
    });

    ready.wait();
    go.count_down();
    t0.join();
    t1.join();
    return (ok0 ? 1 : 0) + (ok1 ? 1 : 0);
}

int multi_pop(BoundedQueue& q) {
    std::latch ready{2};
    std::latch go{1};
    bool ok0 = false;
    bool ok1 = false;

    std::thread t0([&] {
        ready.count_down();
        go.wait();
        int value = 0;
        ok0 = q.try_pop(value);
    });
    std::thread t1([&] {
        ready.count_down();
        go.wait();
        int value = 0;
        ok1 = q.try_pop(value);
    });

    ready.wait();
    go.count_down();
    t0.join();
    t1.join();
    return (ok0 ? 1 : 0) + (ok1 ? 1 : 0);
}

}  // namespace

int main() {
    const char* states[] = {"空", "非满", "满"};
    const char* ops[] = {"单 push", "单 pop", "push+pop", "多 push", "多 pop"};
    int passed = 0;

    // ANCHOR: cases
    for (int state = 0; state < 3; ++state) {
        for (int op = 0; op < 5; ++op) {
            BoundedQueue q;
            prefill(q, state);
            const std::size_t before = q.size();

            bool ok = false;
            int actual = 0;
            int expected = 0;

            if (op == 0) {
                ok = q.try_push(99);
                expected = (state == 2) ? 0 : 1;
                actual = ok ? 1 : 0;
            } else if (op == 1) {
                int value = 0;
                ok = q.try_pop(value);
                expected = (state == 0) ? 0 : 1;
                actual = ok ? 1 : 0;
            } else if (op == 2) {
                const bool pushed = q.try_push(99);
                int value = 0;
                const bool popped = pushed ? q.try_pop(value) : false;
                expected = (state == 2) ? 0 : 1;
                actual = (pushed && popped) ? 1 : 0;
            } else if (op == 3) {
                expected = (state == 0) ? 2 : (state == 1 ? 1 : 0);
                actual = multi_push(q);
            } else {
                expected = (state == 0) ? 0 : (state == 1 ? 1 : 2);
                actual = multi_pop(q);
            }

            std::size_t expected_size = before;
            if (op == 0 || op == 3) {
                expected_size += static_cast<std::size_t>(actual);
            } else if (op == 1 || op == 4) {
                expected_size -= static_cast<std::size_t>(actual);
            }
            const bool good = (actual == expected) && (q.size() == expected_size);
            if (good) {
                ++passed;
                std::printf("%s / %s：通过\n", states[state], ops[op]);
            } else {
                std::printf("%s / %s：失败（预期 %d，实际 %d，大小 %zu）\n",
                            states[state], ops[op], expected, actual, q.size());
            }
        }
    }

    // ANCHOR_END: cases
    std::printf("15 个格子：%d 通过，%d 失败\n", passed, 15 - passed);
    return passed == 15 ? 0 : 1;
}
