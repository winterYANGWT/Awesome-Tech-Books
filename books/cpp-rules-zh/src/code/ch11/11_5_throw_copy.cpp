// 第 11.5 节：异常对象总被复制，按值捕获再切一次，按静态类型抛出会切片
#include <iostream>
#include <string>

struct Payload {
    std::string note = "订单已提交";
    static inline int copies = 0;

    Payload() = default;
    Payload(const Payload& other) : note(other.note) { ++copies; }
};

struct AppError {
    virtual ~AppError() = default;
    virtual std::string what() const { return "AppError"; }
};

struct ValidationError : AppError {
    std::string what() const override { return "ValidationError"; }
};

void throw_by_value() {
    Payload payload;
    Payload::copies = 0;
    try {
        throw payload;                    // 复制一次到异常对象
    } catch (Payload caught) {            // 再复制一次到形参
        std::cout << "catch by value：copies=" << Payload::copies
                  << "，note=" << caught.note << "\n";
    }
}

void throw_by_reference() {
    Payload payload;
    Payload::copies = 0;
    try {
        throw payload;                    // 仍然复制一次到异常对象
    } catch (const Payload& caught) {     // 不再复制
        std::cout << "catch by const&：copies=" << Payload::copies
                  << "，note=" << caught.note << "\n";
    }
}

void throw_dynamic_type() {
    ValidationError local;
    try {
        throw local;                      // 静态类型和动态类型都是 ValidationError
    } catch (const AppError& e) {
        std::cout << "throw local -> " << e.what() << "\n";
    }
}

void throw_static_type() {
    ValidationError local;
    AppError& base = local;               // 引用不改变静态类型
    try {
        throw base;                       // 复制的是 AppError 部分，派生部分被切掉
    } catch (const AppError& e) {
        std::cout << "throw AppError& -> " << e.what() << "\n";
    }
}

int main() {
    throw_by_value();
    throw_by_reference();
    throw_dynamic_type();
    throw_static_type();
}
