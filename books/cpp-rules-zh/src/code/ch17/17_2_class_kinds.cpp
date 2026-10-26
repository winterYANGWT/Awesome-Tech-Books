// 第 17.2 节：先确定写的是值类型、抽象接口还是可组合的具体类
#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

// 值类型：像内置类型一样复制，公开特殊成员函数，没有虚函数，也不打算被继承。
class Money final {
public:
    constexpr explicit Money(long cents) : cents_(cents) {}
    constexpr long cents() const { return cents_; }
    friend constexpr bool operator==(Money, Money) = default;

private:
    long cents_;
};

// 与类型同处的自由函数也属于它的接口；不需要为了“加功能”去派生。
std::string format(Money money) {
    return "$" + std::to_string(money.cents() / 100) + "." +
           (money.cents() % 100 < 10 ? "0" : "") + std::to_string(money.cents() % 100);
}

// 层次结构的根：只有纯虚函数、没有数据成员，析构函数公开且虚，允许多态删除。
class Formatter {
public:
    virtual ~Formatter() = default;
    virtual std::string format(Money money) const = 0;
};

class CentsFormatter final : public Formatter {
public:
    std::string format(Money money) const override {
        return std::to_string(money.cents()) + " cents";
    }
};

// 具体类：通过包含复用 std::string，而不是从 std::string 派生。
class Text final {
public:
    explicit Text(std::string value) : value_(std::move(value)) {}
    void append(const std::string& suffix) { value_ += suffix; }
    const std::string& str() const { return value_; }

private:
    std::string value_;
};

std::string shouted(const Text& text) {
    std::string result = text.str();
    for (char& ch : result) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return result;
}

int main() {
    static_assert(!std::is_polymorphic_v<Money>);
    static_assert(std::is_final_v<Text>);
    static_assert(std::is_abstract_v<Formatter>);

    const Money price(1250);
    std::cout << "值类型 + 自由函数：" << format(price) << '\n';

    std::unique_ptr<Formatter> formatter = std::make_unique<CentsFormatter>();
    std::cout << "抽象接口 + 多态调用：" << formatter->format(price) << '\n';

    Text text("hello");
    text.append(" world");
    std::cout << "包含 + 自由函数：" << shouted(text) << '\n';
}
