// 第 11.2 节：析构函数默认 noexcept，异常一逃出就 std::terminate
// mode: crash
// expect: terminate
#include <iostream>
#include <stdexcept>

// 普通函数：编译器不会在调用点检查它会不会抛
[[noreturn]] void flush_failed() {
    throw std::runtime_error("把缓冲刷到磁盘失败");
}

struct Session {
    ~Session() {
        std::cout << "~Session 开始清理\n";
        flush_failed();                   // 析构函数默认 noexcept(true)
    }
};

int main() {
    std::cout << "离开作用域，没有其他异常在传播\n";
    try {
        Session session;
    } catch (const std::exception& e) {
        // 永远到不了这里：~Session 是 noexcept(true)
        std::cout << "捕获到 " << e.what() << "\n";
    }
}
