// 第 11.4 节：function try block 只能翻译异常，不能把构造失败吞掉
#include <iostream>
#include <stdexcept>
#include <string>

struct ConnectionError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Socket {
    bool open = false;

    explicit Socket(bool fail) {
        if (fail) {
            throw std::runtime_error("socket 打开失败");
        }
        open = true;
    }
    bool is_open() const { return open; }
};

class Connection {
public:
    explicit Connection(bool fail) try : socket_(fail) {
        std::cout << "  Connection 构造函数体\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  handler 捕获：" << e.what() << "\n";
        // 翻译成自己的异常类型；原来的 runtime_error 被替换
        throw ConnectionError(std::string("连接失败：") + e.what());
    }

    bool is_open() const { return socket_.is_open(); }

private:
    Socket socket_;
};

class FallThrough {
public:
    explicit FallThrough(bool fail) try : socket_(fail) {
    } catch (const std::runtime_error& e) {
        std::cout << "  FallThrough handler 记录：" << e.what()
                  << "，然后走到 catch 末尾\n";
        // 这里不 throw；构造函数的 handler 走到末尾会自动重抛原异常
    }

private:
    Socket socket_;
};

int main() {
    std::cout << "--- 翻译成另一种异常 ---\n";
    try {
        Connection conn(true);
    } catch (const ConnectionError& e) {
        std::cout << "  调用方捕获 ConnectionError：" << e.what() << "\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  调用方捕获原异常：" << e.what() << "\n";
    }

    std::cout << "--- handler 不抛，也不能吞掉 ---\n";
    try {
        FallThrough conn(true);
    } catch (const std::runtime_error& e) {
        std::cout << "  调用方仍捕获 runtime_error：" << e.what() << "\n";
    }

    std::cout << "--- 构造成功时 ---\n";
    Connection ok(false);
    std::cout << "  is_open = " << ok.is_open() << "\n";
}
