// 第 13.3 节：what() 返回局部 string 的 c_str()，缓冲区随函数返回销毁
// mode: crash
// sanitize: address
// flags: -Wno-return-stack-address
// expect: heap-use-after-free
#include <exception>
#include <iostream>
#include <string>

class TempError : public std::exception {
public:
    const char* what() const noexcept override {
        std::string message = "temporary message built in what()";
        return message.c_str();
    }
};

int main() {
    try {
        throw TempError{};
    } catch (const std::exception& e) {
        std::cout << "准备读取 what()\n";
        std::cout << e.what() << "\n";
    }
}
