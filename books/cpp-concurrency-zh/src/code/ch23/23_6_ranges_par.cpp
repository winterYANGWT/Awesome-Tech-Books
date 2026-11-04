// mode: fail
// std: c++26
// expect: no match

#include <algorithm>
#include <execution>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3, 4};
    // C++26 已给多数 ranges 算法加策略重载，但本机 libstdc++ 15 没实现。
    std::ranges::for_each(std::execution::par, v, [](int) {});
}
