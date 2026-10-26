// 第 27.1 节：list 的迭代器不是随机访问，std::sort 不能用在它身上
// mode: fail
// expect: invalid operands
#include <algorithm>
#include <list>

int main() {
    std::list<int> values{3, 1, 2};
    std::sort(values.begin(), values.end());
}
