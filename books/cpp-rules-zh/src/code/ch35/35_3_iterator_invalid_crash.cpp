// 第 35.3 节：push_back 让 vector 重新分配，旧迭代器失效
// mode: crash
// expect: heap-use-after-free
#include <iostream>
#include <vector>

int main() {
    std::vector<int> readings{1, 2, 3};
    auto first = readings.begin();
    std::cout << "before push: " << *first << '\n';

    readings.push_back(4);  // 容量不够，元素搬到新缓冲区
    std::cout << "after push: " << *first << '\n';
}
