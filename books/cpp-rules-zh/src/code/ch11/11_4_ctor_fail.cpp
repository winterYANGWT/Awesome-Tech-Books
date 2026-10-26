// 第 11.4 节：成员构造失败时，只销毁已经构造完成的子对象，不调用对象自己的析构
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

struct Trace {
    std::string name;
    bool fail;

    explicit Trace(std::string n, bool should_fail = false)
        : name(std::move(n)), fail(should_fail) {
        std::cout << "  构造 " << name << "\n";
        if (fail) {
            throw std::runtime_error(name + " 构造失败");
        }
    }

    ~Trace() { std::cout << "  析构 " << name << "\n"; }
};

struct Base {
    Base() { std::cout << "  构造 Base\n"; }
    ~Base() { std::cout << "  析构 Base\n"; }
};

class Report : public Base {
public:
    Report() : first_("first"), second_("second", true) {
        std::cout << "  Report 构造函数体（不会执行到这里）\n";
    }

    ~Report() { std::cout << "  析构 Report（不会执行）\n"; }

private:
    Trace first_;
    Trace second_;
};

int main() {
    try {
        Report report;
    } catch (const std::exception& e) {
        std::cout << "main 捕获：" << e.what() << "\n";
    }
    std::cout << "try 块结束，report 从来没有成为一个对象\n";
}
