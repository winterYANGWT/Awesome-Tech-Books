// 第 21.3 节：concept 只检查表达式合法，不检查运算的语义
#include <concepts>
#include <iostream>
#include <vector>

template <typename T>
concept Addable = requires(const T& lhs, const T& rhs) {
    { lhs + rhs } -> std::same_as<T>;
};

template <Addable T>
T sum(const std::vector<T>& values) {
    // 示例约定 values 非空，把第一个元素当作初值。
    T total = values.front();
    for (std::size_t i = 1; i < values.size(); ++i) {
        total = total + values[i];
    }
    return total;
}

struct Vector2 {
    double x;
    double y;
};

Vector2 operator+(const Vector2& lhs, const Vector2& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

std::ostream& operator<<(std::ostream& os, const Vector2& value) {
    return os << '(' << value.x << ", " << value.y << ')';
}

struct Ledger {
    int balance;
};

// 这个 operator+ 通过 concept 检查，但它做的是减法。
Ledger operator+(const Ledger& lhs, const Ledger& rhs) {
    return {lhs.balance - rhs.balance};
}

std::ostream& operator<<(std::ostream& os, const Ledger& value) {
    return os << value.balance;
}

int main() {
    const std::vector<Vector2> path{{1, 2}, {3, 4}};
    std::cout << "路径和: " << sum(path) << '\n';

    const std::vector<Ledger> accounts{{100}, {30}};
    std::cout << "账本“和”: " << sum(accounts) << '\n';

    // 两个类型都满足约束，编译器无法知道 Ledger 的 + 算错了。
    static_assert(Addable<Vector2>);
    static_assert(Addable<Ledger>);
}
