// 第 21.1 节：用 void_t 把“能相加”藏进模板参数，传错类型时只看到替换失败
// mode: fail
// expect: no matching function
#include <string>
#include <type_traits>
#include <utility>

// C++17 的检测惯用法：如果 T + T 不合法，void_t 里的类型就不存在。
template <typename T,
          typename = std::void_t<decltype(std::declval<T>() + std::declval<T>())>>
T add(const T& lhs, const T& rhs) {
    return lhs + rhs;
}

struct Order {
    std::string id;
};

int main() {
    add(Order{"A-1"}, Order{"A-2"});
}
