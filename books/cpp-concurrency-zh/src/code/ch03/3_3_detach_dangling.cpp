// mode: crash
// sanitize: address
// flags: -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
// expect: stack-use-after-scope
// detach 后线程仍按引用访问已经返回的局部数组。
#include <array>
#include <chrono>
#include <cstdio>
#include <thread>

namespace {

void spawn_worker() {
    std::array<int, 4> data{1, 2, 3, 4};
    std::thread worker([&data] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        volatile int sum = data[0] + data[3];
        (void)sum;
    });
    worker.detach();
}  // data 在这里销毁，但分离线程还持有它的引用

}  // namespace

int main() {
    spawn_worker();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return 0;
}
