// mode: crash
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，进程在报告前就终止）
// expect: terminate called after throwing

#include <algorithm>
#include <cstdio>
#include <execution>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<int> values(1'000'000, 0);
    values[424242] = 1;

    std::printf("before for_each(par)\n");
    std::fflush(stdout);
    std::for_each(std::execution::par, values.begin(), values.end(), [](int x) {
        if (x == 1) {
            throw std::runtime_error("boom");
        }
    });
    std::printf("after for_each(par)\n");
}
