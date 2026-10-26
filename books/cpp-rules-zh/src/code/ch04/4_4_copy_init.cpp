// 第 4.4 节：直接初始化、拷贝初始化和赋值分别调用哪个函数
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

struct Record {
    static inline int convert = 0;
    static inline int copy = 0;
    static inline int move = 0;
    static inline int assign = 0;

    std::string value;

    Record(const char* text) : value(text) { ++convert; }
    Record(const Record& other) : value(other.value) { ++copy; }
    Record(Record&& other) noexcept : value(std::move(other.value)) { ++move; }
    Record& operator=(const Record& other) {
        value = other.value;
        ++assign;
        return *this;
    }
    Record& operator=(Record&& other) noexcept {
        value = std::move(other.value);
        ++assign;
        return *this;
    }

    static void reset() { convert = copy = move = assign = 0; }
    static void report(const char* label) {
        std::cout << label << "：转换构造 " << convert << "，拷贝构造 " << copy
                  << "，移动构造 " << move << "，赋值 " << assign << "\n";
    }
};

struct Ticket {
    int number;
    explicit Ticket(int n) : number(n) {}
};

int main() {
    static_assert(std::is_constructible_v<Ticket, int>);
    static_assert(!std::is_convertible_v<int, Ticket>,
                  "explicit 构造函数不参与拷贝初始化");

    Record::reset();
    std::cout << "--- 直接初始化与拷贝初始化 ---\n";
    Record a("A");              // 直接初始化：转换构造函数
    Record b(a);                // 直接初始化：拷贝构造
    Record c = a;               // 拷贝初始化：仍然是拷贝构造，不是赋值
    Record d = "D";             // 拷贝初始化：C++17 起直接构造，不再需要拷贝构造
    Record e = Record("E");     // 纯右值直接落到 e，C++17 保证复制消除
    Record::report("构造完成");

    std::cout << "--- 表达式中的 = 才是赋值 ---\n";
    c = b;                      // 已存在的对象被改写：调用 operator=
    e = std::move(d);           // 移动赋值
    Record::report("赋值之后");

    std::cout << "c = " << c.value << "，e = " << e.value << "\n";

    // 拷贝初始化不能使用 explicit 构造函数，直接初始化可以
    Ticket ticket(42);
    std::cout << "ticket = " << ticket.number << "\n";
}
