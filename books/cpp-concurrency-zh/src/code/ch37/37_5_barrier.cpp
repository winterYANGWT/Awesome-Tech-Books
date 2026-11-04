// mode: run
// expect: 完成函数: 阶段 1 结束
// expect: 完成函数: 阶段 2 结束
// expect: 阶段 1: 0 1 2
// expect: 阶段 2: 0 1
// expect: 完成函数调用: 2 次
#include <algorithm>
#include <barrier>
#include <chrono>
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

static std::mutex phase_mu;
static int phase = 0;

struct PhaseCompletion {
    void operator()() const noexcept {
        std::lock_guard<std::mutex> lk(phase_mu);
        ++phase;
        std::cout << "完成函数: 阶段 " << phase << " 结束\n";
    }
};

static std::barrier<PhaseCompletion> bar{3, PhaseCompletion{}};

static std::mutex record_mu;
static std::vector<int> participants[2];

static void worker(int id, std::latch &start, std::counting_semaphore<2> &slots) {
    start.wait();  // 起跑门：等工作线程把 latch 打开
    for (int p = 0; p < 2; ++p) {
        {
            std::lock_guard<std::mutex> lk(record_mu);
            participants[p].push_back(id);
        }

        slots.acquire();  // 最多两个线程同时做这一阶段的工作
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        slots.release();

        if (p == 0 && id == 2) {
            bar.arrive_and_drop();  // 线程 2 只参加第一阶段
            return;
        }
        bar.arrive_and_wait();
    }
}

int main() {
    std::latch start{1};
    std::counting_semaphore<2> slots{2};
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(worker, i, std::ref(start), std::ref(slots));
    }
    start.count_down();  // 起跑门打开，三个线程同时开始
    for (auto &t : threads) {
        t.join();
    }

    for (int p = 0; p < 2; ++p) {
        std::vector<int> &v = participants[p];
        std::sort(v.begin(), v.end());
        std::cout << "阶段 " << (p + 1) << ":";
        for (int id : v) {
            std::cout << " " << id;
        }
        std::cout << "\n";
    }
    std::cout << "完成函数调用: " << phase << " 次\n";
    return 0;
}
