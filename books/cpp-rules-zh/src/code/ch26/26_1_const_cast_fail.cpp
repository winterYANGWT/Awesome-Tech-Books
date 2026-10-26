// 第 26.1 节：const_iterator 不能 const_cast 成 iterator，两者是不同的类型
// mode: fail
// expect: const_cast
#include <vector>

int main() {
    std::vector<int> readings{1, 2, 3};
    auto ci = readings.cbegin();
    auto i = const_cast<std::vector<int>::iterator>(ci);
    *i = 9;
}
