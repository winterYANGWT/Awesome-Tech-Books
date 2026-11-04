// mode: fail
// expect: experimental/future|No such file
// Concurrency TS 的 future::then 在 libstdc++ 15 里没有头文件。
#include <cstdio>
#include <experimental/future>

int main() {
    auto seed = std::experimental::make_ready_future(1);
    auto next = seed.then([](std::experimental::future<int> done) {
        return done.get() + 1;
    });
    std::printf("结果: %d\n", next.get());
}
