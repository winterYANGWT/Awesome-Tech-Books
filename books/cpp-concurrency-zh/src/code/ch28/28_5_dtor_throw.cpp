// mode: crash
// expect: terminate called after throwing
// 反例：析构函数在栈展开期间抛异常，std::terminate 被调用。
#include <cstdio>
#include <stdexcept>

struct close_queue {};

struct Dispatcher {
    // 旧写法：显式声明 noexcept(false)，并在析构里抛 close_queue。
    ~Dispatcher() noexcept(false) { throw close_queue{}; }
};

void process_message() {
    Dispatcher guard;
    throw std::runtime_error("消息处理失败");
}

int main() {
    try {
        process_message();
    } catch (const std::exception& e) {
        std::printf("捕获：%s\n", e.what());
    }
}
