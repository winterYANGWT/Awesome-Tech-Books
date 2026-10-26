// 第 26.3 节：关联容器的键是 const，std::remove 无法给元素赋值
// mode: fail
// expect: deleted operator '='
#include <algorithm>
#include <map>

int main() {
    std::map<int, int> stock{{1, 10}, {2, 20}, {3, 5}};
    std::remove_if(stock.begin(), stock.end(),
                   [](const auto& entry) { return entry.second < 15; });
}
