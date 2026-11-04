// mode: run
// expect: 总和 = 400000
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    constexpr int kElements = 4;
    constexpr int kRounds = 100'000;
    std::vector<int> values(kElements, 0);

    auto add = [&](int first) {
        for (int round = 0; round < kRounds; ++round) {
            for (int i = first; i < kElements; i += 2) {
                // 对象还是普通 int，只是这段时间内通过 atomic_ref 访问
                std::atomic_ref<int> ref(values[i]);
                ref.fetch_add(1);
            }
        }
    };

    std::thread a(add, 0);
    std::thread b(add, 1);
    a.join();
    b.join();

    long long total = 0;
    for (int v : values) {
        total += v;
    }
    std::cout << "总和 = " << total << '\n';
}
