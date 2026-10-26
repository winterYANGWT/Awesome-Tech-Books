// 第 32.2 节：改名宏会悄悄替换同名函数，重载决议跟着变
#include <iostream>
#include <string>

// 模拟厂商头文件：旧 API 和新 API 同时存在。
int log(const std::string& msg) {
    std::cout << "旧 API log: " << msg << '\n';
    return 1;
}
int log_v2(const std::string& msg, int level) {
    std::cout << "新 API log_v2(level=" << level << "): " << msg << '\n';
    return level;
}

// 厂商为了兼容，加了一条“把旧名字换成新名字”的宏。
// 它不认类型、不认作用域，从这一行到文件末尾，所有 log 都变成 log_v2。
#define log log_v2

// 我们想写一个只收消息的 log；它其实变成了 log_v2 的又一个重载。
int log(const std::string& msg) {
    std::cout << "我们的重载，真实名字是 " << __func__ << ": " << msg << '\n';
    return 0;
}

struct Logger {
    void log(const char* msg) const {
        std::cout << "成员函数，真实名字是 " << __func__ << ": " << msg << '\n';
    }
};

int main() {
    log(std::string("one arg"));       // 实参一个，选中我们的重载
    log(std::string("two args"), 2);   // 实参两个，选中厂商的新 API
    Logger{}.log("member");            // 成员函数也被改了名

#undef log
    log(std::string("after undef"));   // 取消宏之后，真的是旧 API
}
