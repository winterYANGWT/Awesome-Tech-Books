// 第 21.1 节：同一条“能相加”的约束写成 requires，诊断直接说明哪个表达式不合法
// mode: fail
// expect: constraints not satisfied
#include <string>

template <typename T>
    requires requires(const T& lhs, const T& rhs) { lhs + rhs; }
T add(const T& lhs, const T& rhs) {
    return lhs + rhs;
}

struct Order {
    std::string id;
};

int main() {
    add(Order{"A-1"}, Order{"A-2"});
}
