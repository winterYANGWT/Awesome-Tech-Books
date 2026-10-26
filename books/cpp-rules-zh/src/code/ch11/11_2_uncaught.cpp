// 第 11.2 节：std::uncaught_exceptions 用来识别栈展开，不是用来决定“能不能抛”
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

// 只有“构造之后没有开始新的栈展开”时才提交，否则回滚
class Transaction {
public:
    explicit Transaction(std::string name)
        : name_(std::move(name)), baseline_(std::uncaught_exceptions()) {}

    ~Transaction() {
        const int now = std::uncaught_exceptions();
        if (now > baseline_) {
            std::cout << "  " << name_ << "：检测到新的栈展开，回滚（计数 "
                      << baseline_ << " -> " << now << "）\n";
        } else {
            std::cout << "  " << name_ << "：正常提交（计数 " << now << "）\n";
        }
    }

private:
    std::string name_;
    int baseline_;
};

void handled_inside() {
    Transaction outer("outer");
    try {
        Transaction inner("inner");
        throw std::runtime_error("内部失败");
    } catch (const std::exception& e) {
        std::cout << "  处理掉：" << e.what()
                  << "，此时 uncaught_exceptions=" << std::uncaught_exceptions() << "\n";
    }
    std::cout << "  inner 已析构，outer 还没到析构点\n";
}

void propagate_out() {
    Transaction outer("propagating");
    throw std::runtime_error("传给调用者");
}

int main() {
    std::cout << "--- 异常在内部被捕获 ---\n";
    handled_inside();

    std::cout << "--- 异常继续向外传播 ---\n";
    try {
        propagate_out();
    } catch (const std::exception& e) {
        std::cout << "  最外层捕获：" << e.what()
                  << "，此时 uncaught_exceptions=" << std::uncaught_exceptions() << "\n";
    }
}
