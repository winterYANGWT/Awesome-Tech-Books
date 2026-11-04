// mode: run
// expect: system_clock: is_steady=false
// expect: steady_clock: is_steady=true
// expect: high_resolution_clock 与 system_clock 同类型: 是
// 只打印三种时钟的稳定性和节拍，不打印当前时间。
#include <chrono>
#include <cstdio>
#include <type_traits>

namespace {

template <class Clock>
void print_clock(const char* name) {
    std::printf("%s: is_steady=%s, period=%lld/%lld\n", name,
                Clock::is_steady ? "true" : "false",
                static_cast<long long>(Clock::period::num),
                static_cast<long long>(Clock::period::den));
}

}  // namespace

int main() {
    using std::chrono::high_resolution_clock;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    print_clock<system_clock>("system_clock");
    print_clock<steady_clock>("steady_clock");
    print_clock<high_resolution_clock>("high_resolution_clock");
    std::printf("high_resolution_clock 与 system_clock 同类型: %s\n",
                std::is_same_v<high_resolution_clock, system_clock> ? "是" : "否");
}
