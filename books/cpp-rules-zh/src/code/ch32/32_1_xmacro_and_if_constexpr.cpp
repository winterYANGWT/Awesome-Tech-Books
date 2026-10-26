// 第 32.1 节：预处理器仍适合生成重复数据和探测编译环境，调试分支改用 if constexpr
#include <iostream>
#include <string_view>
#include <version>

// 一份数据，多处生成：枚举、名字表都从这张表来。
// 这是 MXC-34 所说的“变体数据表示”，也是 X-macro 的典型用法。
#define ORDER_STATES(X)      \
    X(Pending, "pending")    \
    X(Paid, "paid")          \
    X(Shipped, "shipped")    \
    X(Cancelled, "cancelled")

enum class OrderState {
#define X(name, text) name,
    ORDER_STATES(X)
#undef X
};

constexpr std::string_view to_string(OrderState s) {
    switch (s) {
#define X(name, text) case OrderState::name: return text;
        ORDER_STATES(X)
#undef X
    }
    return "unknown";
}

// 调试开关写成 constexpr，整份程序只编译一次，两个分支都要通过语法和类型检查。
constexpr bool kTrace = false;

int main() {
    for (OrderState s : {OrderState::Pending, OrderState::Paid,
                         OrderState::Shipped, OrderState::Cancelled}) {
        std::cout << to_string(s) << ' ';
    }
    std::cout << '\n';

    if constexpr (kTrace) {
        std::cout << "trace: kTrace 为 false 时这段不会生成\n";
    }

    // 探测标准库能力是预处理器至今无可替代的用途。
#if defined(__cpp_lib_source_location)
    std::cout << "source_location: yes\n";
#else
    std::cout << "source_location: no\n";
#endif
    std::cout << "__cplusplus=" << __cplusplus << '\n';
}
