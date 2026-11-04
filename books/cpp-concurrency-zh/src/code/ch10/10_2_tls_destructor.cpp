// mode: run
// output: varies
// expect: 构造 A0[\s\S]*析构 A0
#include <barrier>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

struct Probe {
    std::string name;
    explicit Probe(std::string n) : name(std::move(n)) {
        std::printf("构造 %s\n", name.c_str());
    }
    ~Probe() { std::printf("析构 %s\n", name.c_str()); }
};

std::barrier gate(2);

void worker(int id) {
    const std::string a = "A" + std::to_string(id);
    const std::string b = "B" + std::to_string(id);
    // 块作用域 thread_local：控制流第一次经过声明时构造，每个线程各一次。
    thread_local Probe x(a);
    thread_local Probe y(b);
    (void)x;
    (void)y;
    gate.arrive_and_wait();
    std::printf("使用 %s %s\n", a.c_str(), b.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

int main() {
    std::thread t0(worker, 0);
    std::thread t1(worker, 1);
    t0.join();
    t1.join();
}
