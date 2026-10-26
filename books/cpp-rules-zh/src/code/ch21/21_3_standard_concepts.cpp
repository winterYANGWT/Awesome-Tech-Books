// 第 21.3 节：标准库提供的 concepts 覆盖常见接口要求
#include <algorithm>
#include <concepts>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <vector>

struct Instrument {
    virtual ~Instrument() = default;
    virtual std::string ticker() const = 0;
};

struct Equity : Instrument {
    std::string ticker() const override { return "EQ-001"; }
};

struct Bond : Instrument {
    std::string ticker() const override { return "BD-002"; }
};

// std::derived_from 检查公开、无歧义的继承关系。
template <std::derived_from<Instrument> T>
void registerInstrument(const T& instrument) {
    std::cout << "注册 " << instrument.ticker() << '\n';
}

// std::integral 与 std::floating_point 正好把数值类型分成两组。
template <std::integral T>
T twice(T value) {
    return value * 2;
}

template <std::floating_point T>
T twice(T value) {
    return value * 2;
}

// std::same_as 要求类型完全相同，不做隐式转换。
template <typename T>
    requires std::same_as<T, std::string>
std::size_t textLength(const T& text) {
    return text.size();
}

template <typename T>
concept HasTextLength = requires(T value) { textLength(value); };

// std::invocable 检查“能不能用这些实参调用”，返回类型交给 invoke_result_t。
template <std::invocable<int> F>
auto apply(F&& f, int value) -> std::invoke_result_t<F, int> {
    return std::invoke(std::forward<F>(f), value);
}

// std::predicate 要求返回值能当布尔值用。
template <std::predicate<int> P>
std::size_t countMatching(const std::vector<int>& values, P predicate) {
    return static_cast<std::size_t>(
        std::count_if(values.begin(), values.end(), predicate));
}

bool isEven(int value) {
    return value % 2 == 0;
}

int main() {
    Equity equity;
    Bond bond;
    registerInstrument(equity);
    registerInstrument(bond);

    std::cout << "twice(21) = " << twice(21) << '\n';
    std::cout << "twice(1.25) = " << twice(1.25) << '\n';
    std::cout << "textLength = " << textLength(std::string("ledger")) << '\n';
    std::cout << "apply = " << apply([](int value) { return std::to_string(value * 3); }, 7) << '\n';

    const std::vector<int> values{1, 2, 3, 4, 5, 6};
    std::cout << "偶数个数 = " << countMatching(values, isEven) << '\n';
    std::cout << "大于 3 的个数 = "
              << countMatching(values, [](int value) { return value > 3; }) << '\n';

    static_assert(std::derived_from<Equity, Instrument>);
    static_assert(!std::derived_from<Instrument, Equity>);
    static_assert(std::integral<int>);
    static_assert(!std::integral<double>);
    static_assert(std::floating_point<double>);
    static_assert(std::same_as<decltype(twice(21)), int>);
    static_assert(std::same_as<decltype(twice(1.25)), double>);
    static_assert(std::invocable<decltype(&isEven), int>);
    static_assert(std::predicate<decltype(&isEven), int>);
    static_assert(!std::same_as<std::string, std::string_view>);
    static_assert(HasTextLength<std::string>);
    static_assert(!HasTextLength<const char*>);
}
