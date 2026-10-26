// 第 20.2 节：派生类模板不会自动查找依赖基类里的名字
#include <iostream>
#include <string>
#include <type_traits>

template<class T>
struct Sender {
    using message_type = T;

    virtual void send(const T& msg) const {
        std::cout << "Sender::send(" << msg << ")\n";
    }

    void log() const { std::cout << "Sender::log\n"; }
};

template<class T>
struct LoggingSender : Sender<T> {
    // 基类里的依赖 typedef 也要 typename，再用 using 变成非依赖名
    using message_type = typename Sender<T>::message_type;

    void send(const T& msg) const override {
        std::cout << "LoggingSender::send(" << msg << ")\n";
    }

    // 方式一：this-> 让名字进入依赖基类
    void via_this(const T& msg) const { this->send(msg); }

    // 方式二：using 声明把基类名字带进来
    using Sender<T>::log;
    void via_using() const { log(); }

    // 方式三：显式限定，虚分发会被关掉
    void via_qualified(const T& msg) const { Sender<T>::send(msg); }

    void check_type() const {
        static_assert(std::is_same_v<message_type, T>);
        std::cout << "message_type = T\n";
    }
};

int main() {
    LoggingSender<std::string> sender;
    sender.via_this("hello");       // 虚调用，落到 LoggingSender
    sender.via_using();             // 普通成员，落到 Sender
    sender.via_qualified("hello");  // 限定调用，落到 Sender
    sender.check_type();
}
