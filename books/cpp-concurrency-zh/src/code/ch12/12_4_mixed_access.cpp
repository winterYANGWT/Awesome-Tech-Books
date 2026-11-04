// mode: race
// expect: data race
#include <atomic>
#include <iostream>
#include <thread>

int main() {
    int value = 0;
    std::atomic<bool> start{false};
    std::atomic<long long> sink{0};

    auto writer = [&] {
        while (!start.load()) {
        }
        for (int i = 0; i < 100'000; ++i) {
            std::atomic_ref<int> ref(value);
            ref.fetch_add(1);  // 原子写
        }
    };
    auto reader = [&] {
        while (!start.load()) {
        }
        long long sum = 0;
        for (int i = 0; i < 100'000; ++i) {
            sum += value;  // 普通读：和上面的原子写没有先行发生关系
        }
        sink.store(sum);
    };

    std::thread a(writer);
    std::thread b(reader);
    start.store(true);
    a.join();
    b.join();

    std::cout << "value = " << value << "，sink = " << sink.load() << '\n';
}
