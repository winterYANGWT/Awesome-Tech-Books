// 第 2.5 节：enum class 把枚举名关进作用域，并拒绝隐式转整数
#include <cstdint>
#include <iostream>
#include <type_traits>

enum class OrderState : std::uint8_t {
    pending, paid, shipped, cancelled
};

const char* to_string(OrderState s) {
    switch (s) {
    case OrderState::pending:   return "pending";
    case OrderState::paid:      return "paid";
    case OrderState::shipped:   return "shipped";
    case OrderState::cancelled: return "cancelled";
    }
    return "unknown";
}

int main() {
    OrderState state = OrderState::paid;
    std::cout << to_string(state) << '\n';

    // 底层类型和大小都由类型本身说清楚，适合放进协议或文件头。
    static_assert(std::is_same_v<std::underlying_type_t<OrderState>, std::uint8_t>);
    static_assert(sizeof(OrderState) == sizeof(std::uint8_t));

    // 不会像 C 风格 enum 那样悄悄变成 int。
    static_assert(!std::is_convertible_v<OrderState, int>);

    // C++20 的 using enum 把枚举名引入当前作用域，仍然保留类型检查。
    using enum OrderState;
    OrderState next = shipped;
    std::cout << to_string(next) << '\n';
    std::cout << "underlying = "
              << static_cast<int>(static_cast<std::underlying_type_t<OrderState>>(next))
              << '\n';
}
