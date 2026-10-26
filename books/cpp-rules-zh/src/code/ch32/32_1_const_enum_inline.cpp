// 第 32.1 节：常量、枚举和 inline 函数能表达的事，不必交给 #define
#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

// 常量宏：没有类型，名字在预处理后就不存在了。
#define TAX_RATE 0.08

// 有类型的常量：编译器看得见名字、类型和作用域。
constexpr double kTaxRate = 0.08;
static_assert(std::is_same_v<decltype(kTaxRate), const double>);

// 函数宏：实参在宏体里出现两次，传一个带副作用的表达式就会被求值两次。
#define NET_PRICE(price) ((price) + (price) * TAX_RATE)
inline double net_price(double price) { return price + price * kTaxRate; }

// inline 函数可以重载；宏没有类型，谈不上重载。
inline std::string describe(int) { return "describe(int)"; }
inline std::string describe(double) { return "describe(double)"; }

// 类作用域常量：C++17 起 static constexpr 成员隐式 inline，
// 取地址也不必再在类外补一行定义。
struct Warehouse {
    static constexpr std::size_t kMaxItems = 8;
    std::array<int, kMaxItems> slots{};
};

// 枚举类有独立类型，不能悄悄当整数用。
enum class OrderState { pending, paid, shipped };
static_assert(!std::is_convertible_v<OrderState, int>);

int price_calls = 0;
double next_price() {
    ++price_calls;
    return 100.0;
}

int main() {
    std::cout << "宏求值: " << NET_PRICE(next_price())
              << "，next_price 调用 " << price_calls << " 次\n";
    price_calls = 0;
    std::cout << "函数求值: " << net_price(next_price())
              << "，next_price 调用 " << price_calls << " 次\n";

    std::cout << describe(3) << " / " << describe(3.5) << '\n';

    const std::size_t* addr = &Warehouse::kMaxItems;
    std::cout << "通过指针读 kMaxItems: " << *addr << '\n';

    const OrderState state = OrderState::paid;
    std::cout << "paid 的值: " << static_cast<int>(state) << '\n';
}
