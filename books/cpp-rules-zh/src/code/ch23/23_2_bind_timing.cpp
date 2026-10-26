// 第 23.2 节：bind 在创建时求值普通实参，lambda 在调用时求值
#include <cstdio>
#include <functional>

// 模拟“当前时刻”：每调用一次前进一格
int clockTick() {
    static int tick = 0;
    return ++tick;
}

// 在时刻 at 安排一场持续 duration 秒、音量为 volume 的提醒
int scheduleAt(int at, int duration, int volume) {
    return at + duration + volume;
}

void bump(int& value) { ++value; }

int main() {
    using namespace std::placeholders;

    // bind 的普通实参立刻求值：clockTick() 在这里就被调用（tick = 1）
    auto bound = std::bind(scheduleAt, clockTick(), 30, _1);

    // lambda 把 clockTick() 写在函数体里，每次调用才求值
    auto lambda = [](int volume) { return scheduleAt(clockTick(), 30, volume); };

    clockTick();  // tick = 2
    clockTick();  // tick = 3

    std::printf("bind 对象：%d（用的是创建时的 tick=1）\n", bound(5));
    std::printf("lambda：%d（用的是调用时的 tick=4）\n", lambda(5));

    // 想让 bind 也延迟求值，只能再套一层 bind
    auto deferred = std::bind(scheduleAt, std::bind(clockTick), 30, _1);
    std::printf("嵌套 bind：%d（这次是调用时的 tick=5）\n", deferred(5));

    // bind 对象的 operator() 对调用实参做完美转发，_1 位置要的是 int&
    int counter = 0;
    auto bumpBind = std::bind(bump, _1);
    bumpBind(counter);
    std::printf("bind 转发实参后 counter = %d\n", counter);
}
