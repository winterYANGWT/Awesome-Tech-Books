// 第 35.4 节：对空 vector 取 &v[0] 不是“空区间”
// mode: crash
// expect: Assertion
#include <iostream>
#include <vector>

int main() {
    std::vector<int> empty;
    std::cout << "size=" << empty.size() << '\n';

    int* first = &empty[0];  // 下标 0 越界；不是合法的“首地址”
    std::cout << "first=" << *first << '\n';
}
