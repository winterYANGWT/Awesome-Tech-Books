// mode: race
// expect: data race
// 相邻位域是不同对象，却挤在同一个存储单元里：两个线程各写一个，TSan 报同一地址。
#include <cstdio>
#include <thread>

struct Flags {
    unsigned bf1 : 4;
    unsigned bf2 : 4;
};

int main() {
    Flags flags{};
    std::puts("两个线程各写一个相邻位域");
    std::thread a([&] { flags.bf1 = 1; });
    std::thread b([&] { flags.bf2 = 2; });
    a.join();
    b.join();
}
