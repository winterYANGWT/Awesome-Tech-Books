// 第 15.3 节：std::formatter 给出不经过 ostream 的格式化接口
#include <format>
#include <iostream>

namespace billing {

struct Money {
    long cents;
};

}  // namespace billing

// formatter<Money> 的特化必须放在 std 里；它只使用 Money 的公开成员。
namespace std {

template <>
struct formatter<billing::Money> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const billing::Money& money, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}.{:02}", money.cents / 100, money.cents % 100);
    }
};

}  // namespace std

int main() {
    std::cout << std::format("金额：{}\n", billing::Money{4205});
    std::cout << std::format("两笔：{} + {}\n",
                             billing::Money{1999}, billing::Money{250});
}
