// 第 19.3 节：类型码加几张平行表，新增一种类型要同步改很多处
#include <array>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

// 支付方式用类型码表示。增加一种方式时，下面这些地方都要手动同步：
// 1) 枚举 2) kMethodCount 3) 名字表 4) 费率表 5) receipt 的 switch 6) parse 的查找
enum class Method {
    Card,
    Cash,
    Crypto,
};

constexpr std::size_t kMethodCount = 3;
constexpr std::array<const char*, kMethodCount> kNames{"card", "cash", "crypto"};
// 费率以万分之一为单位：250 = 2.5%。
constexpr std::array<int, kMethodCount> kFeeBps{250, 0, 100};

// 这两条只能保证“表的长度和计数一致”，保证不了顺序和语义。
static_assert(kNames.size() == kMethodCount);
static_assert(kFeeBps.size() == kMethodCount);

std::size_t index_of(Method method) {
    return static_cast<std::size_t>(method);
}

int fee_cents(Method method, int amount_cents) {
    return amount_cents * kFeeBps[index_of(method)] / 10000;
}

std::string receipt(Method method, int amount_cents) {
    switch (method) {
    case Method::Card:
        return "card slip: " + std::to_string(amount_cents);
    case Method::Cash:
        return "cash slip: " + std::to_string(amount_cents);
    case Method::Crypto:
        return "crypto slip: " + std::to_string(amount_cents);
    default:
        // 新类型如果忘了加 case，会悄悄走到这里，而不是编译失败。
        return "unknown method";
    }
}

std::optional<Method> parse_method(std::string_view name) {
    for (std::size_t i = 0; i < kMethodCount; ++i) {
        if (name == kNames[i]) {
            return static_cast<Method>(i);
        }
    }
    return std::nullopt;
}

int main() {
    for (std::size_t i = 0; i < kMethodCount; ++i) {
        const Method method = static_cast<Method>(i);
        constexpr int amount = 10000;
        std::cout << kNames[i] << ": fee=" << fee_cents(method, amount)
                  << ", " << receipt(method, amount) << '\n';
    }

    const auto parsed = parse_method("crypto");
    std::cout << "parse crypto: " << (parsed.has_value() ? "ok" : "missing") << '\n';
}
