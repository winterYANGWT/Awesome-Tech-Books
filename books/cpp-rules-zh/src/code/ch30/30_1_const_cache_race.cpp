// 第 30.1 节：两个线程同时调用同一个 const 缓存函数会构成数据竞争
// mode: crash
// sanitize: thread
// flags: -pthread
// expect: data race
#include <barrier>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

class SensorHistory {
public:
    using Samples = std::vector<double>;

    // const 只说明逻辑上不改变对象，不说明它不会写 mutable 成员。
    Samples calibrated() const {
        if (!valid_) {
            for (int i = 0; i < 200000; ++i)
                samples_.push_back(i * 0.5);     // 两个线程会一起改这个 vector
            valid_ = true;
        }
        return samples_;
    }

private:
    mutable bool valid_ = false;
    mutable Samples samples_;
};

int main() {
    const SensorHistory history;
    std::cout << "两个线程同时调用 calibrated()\n";
    std::barrier start(2);

    auto worker = [&] {
        start.arrive_and_wait();
        auto s = history.calibrated();
        std::cout << "线程拿到 " << s.size() << " 个采样\n";
    };
    std::thread t1(worker);
    std::thread t2(worker);
    t1.join();
    t2.join();
}
