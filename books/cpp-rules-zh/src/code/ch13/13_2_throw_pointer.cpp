// 第 13.2 节：抛指针而不是抛值，栈展开后捕获方拿到悬垂指针
// mode: crash
// sanitize: address
// expect: stack-use-after-scope
#include <exception>
#include <iostream>
#include <string>

struct ParseError : std::exception {
    std::string message;
    explicit ParseError(std::string text) : message(std::move(text)) {}
    const char* what() const noexcept override { return message.c_str(); }
};

int main() {
    try {
        ParseError local("unexpected token at line 4");
        throw &local;  // 异常对象只是指针；local 在离开 try 块时被销毁
    } catch (const ParseError* error) {
        std::cout << "捕获到指针，准备读取它指向的对象\n";
        std::cout << error->what() << "\n";
    }
}
