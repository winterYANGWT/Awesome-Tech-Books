// 第 4.1 节：提前默认构造再赋值，和等到有初值再定义，代价差多少
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

// 记录每种特殊成员函数被调用的次数，让“多做了一次默认构造”看得见
struct Payload {
    static inline int default_ctor = 0;
    static inline int value_ctor = 0;
    static inline int move_assign = 0;
    static inline int copy_assign = 0;
    static inline int destroyed = 0;

    std::string text;

    Payload() : text("(空)") {
        ++default_ctor;
        std::cout << "  Payload() 默认构造\n";
    }
    explicit Payload(std::string t) : text(std::move(t)) {
        ++value_ctor;
        std::cout << "  Payload(string) 直接构造: " << text << "\n";
    }
    Payload(const Payload&) = delete;
    Payload(Payload&&) = delete;
    Payload& operator=(const Payload& other) {
        ++copy_assign;
        text = other.text;
        std::cout << "  operator=(const Payload&)\n";
        return *this;
    }
    Payload& operator=(Payload&& other) noexcept {
        ++move_assign;
        text = std::move(other.text);
        other.text = "(moved)";
        std::cout << "  operator=(Payload&&)\n";
        return *this;
    }
    ~Payload() {
        ++destroyed;
        std::cout << "  ~Payload(" << text << ")\n";
    }

    static void reset() {
        default_ctor = value_ctor = move_assign = copy_assign = destroyed = 0;
    }
    static void report(const char* label) {
        std::cout << label << ": 默认构造 " << default_ctor
                  << "，直接构造 " << value_ctor
                  << "，赋值 " << (move_assign + copy_assign)
                  << "，析构 " << destroyed << "\n";
    }
};

// 先默认构造，再检查参数，最后赋值：参数不合法时那次默认构造白做
void encrypt_early(const std::string& raw) {
    Payload p;
    if (raw.size() < 8) {
        throw std::invalid_argument("口令太短");
    }
    p = Payload("cipher:" + raw);
    std::cout << "  加密结果: " << p.text << "\n";
}

// 检查通过、拿到初值之后才定义：异常路径上一个对象都不构造
void encrypt_late(const std::string& raw) {
    if (raw.size() < 8) {
        throw std::invalid_argument("口令太短");
    }
    Payload p("cipher:" + raw);
    std::cout << "  加密结果: " << p.text << "\n";
}

int main() {
    std::cout << "--- 太短的口令，encrypt_early ---\n";
    try {
        encrypt_early("abc");
    } catch (const std::exception& e) {
        std::cout << "  抛出: " << e.what() << "\n";
    }
    Payload::report("提前定义");

    Payload::reset();
    std::cout << "--- 太短的口令，encrypt_late ---\n";
    try {
        encrypt_late("abc");
    } catch (const std::exception& e) {
        std::cout << "  抛出: " << e.what() << "\n";
    }
    Payload::report("推迟定义");

    Payload::reset();
    std::cout << "--- 足够长的口令，encrypt_late ---\n";
    encrypt_late("secret42");
    Payload::report("推迟定义成功");

    Payload::reset();
    std::cout << "--- 足够长的口令，encrypt_early ---\n";
    encrypt_early("secret42");
    Payload::report("提前定义成功");

    // 定义处就给初值：内置类型和聚合都不会留下不确定的位
    int attempts{};
    std::array<int, 3> counters{};
    std::cout << "attempts = " << attempts << "，counters = "
              << counters[0] << counters[1] << counters[2] << "\n";
}
