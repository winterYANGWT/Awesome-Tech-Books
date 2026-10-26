// 第 4.2 节：成员在初始化列表里直接构造，和先默认构造再赋值，差别在哪
#include <iostream>
#include <string>
#include <utility>

// 把每种特殊成员函数的调用次数记下来；具体文本内容不重要
struct Text {
    static inline int default_ctor = 0;
    static inline int value_ctor = 0;
    static inline int copy_ctor = 0;
    static inline int move_ctor = 0;
    static inline int copy_assign = 0;
    static inline int move_assign = 0;

    std::string value;

    Text() : value("(空)") { ++default_ctor; }
    explicit Text(std::string v) : value(std::move(v)) { ++value_ctor; }
    Text(const Text& other) : value(other.value) { ++copy_ctor; }
    Text(Text&& other) noexcept : value(std::move(other.value)) { ++move_ctor; }
    Text& operator=(const Text& other) {
        value = other.value;
        ++copy_assign;
        return *this;
    }
    Text& operator=(Text&& other) noexcept {
        value = std::move(other.value);
        ++move_assign;
        return *this;
    }

    static void reset() {
        default_ctor = value_ctor = copy_ctor = move_ctor = copy_assign = move_assign = 0;
    }
    static void report(const char* label) {
        std::cout << label << "：默认构造 " << default_ctor
                  << "，值构造 " << value_ctor
                  << "，拷贝构造 " << copy_ctor
                  << "，移动构造 " << move_ctor
                  << "，拷贝赋值 " << copy_assign
                  << "，移动赋值 " << move_assign << "\n";
    }
};

// 推荐：初始化列表直接构造
class Account {
public:
    Account(std::string owner, std::string currency)
        : owner_(std::move(owner)), currency_(std::move(currency)) {}

    const std::string& owner() const { return owner_.value; }

private:
    Text owner_;
    Text currency_;
};

// 反例：构造函数体里赋值，成员先被默认构造了一次
class LegacyAccount {
public:
    LegacyAccount(std::string owner, std::string currency) {
        owner_ = Text(std::move(owner));
        currency_ = Text(std::move(currency));
    }

    const std::string& owner() const { return owner_.value; }

private:
    Text owner_;
    Text currency_;
};

int main() {
    Text::reset();
    {
        Account account("Lin", "CNY");
        std::cout << "Account owner = " << account.owner() << "\n";
    }
    Text::report("初始化列表");

    Text::reset();
    {
        LegacyAccount account("Lin", "CNY");
        std::cout << "LegacyAccount owner = " << account.owner() << "\n";
    }
    Text::report("函数体赋值");
}
