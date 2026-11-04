// mode: fail
// flags: -ltbb
// expect: no matching function

#include <execution>
#include <numeric>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3, 4};
    // accumulate 没有执行策略重载：策略版是 reduce。
    auto sum = std::accumulate(std::execution::par, v.begin(), v.end(), 0);
    return sum == 10 ? 0 : 1;
}
