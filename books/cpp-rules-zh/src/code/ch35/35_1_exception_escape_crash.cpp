// 第 35.1 节：异常逃出 noexcept 边界时进程终止
// mode: crash
// expect: terminate called after throwing
#include <iostream>
#include <stdexcept>

// 插件用异常报告错误；extern "C" 只改链接名，不会替它挡住异常。
extern "C" void plugin_tick() {
    throw std::runtime_error("sensor driver offline");
}

using tick_fn = void (*)();

// 宿主把回调入口标成 noexcept，表示“这个边界不抛”。
void host_loop(tick_fn tick) noexcept {
    tick();
}

int main() {
    std::cout << "host starts\n";
    host_loop(plugin_tick);
    std::cout << "host survives\n";
}
