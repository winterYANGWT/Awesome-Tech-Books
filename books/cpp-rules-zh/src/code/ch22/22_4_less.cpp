// 第 22.4 节：用 < 比较迭代器，把代码绑在随机访问迭代器上
// mode: fail
// expect: invalid operands to binary expression
#include <list>

template <class It>
int count_steps(It first, It last) {
    int steps = 0;
    for (; first < last; ++first) ++steps;  // list 迭代器没有 <
    return steps;
}

int main() {
    std::list<int> pending{1, 2, 3};
    return count_steps(pending.begin(), pending.end());
}
