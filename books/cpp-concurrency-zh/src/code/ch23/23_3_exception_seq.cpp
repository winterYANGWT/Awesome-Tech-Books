// mode: crash
// flags: -ltbb
// expect: terminate called after throwing

#include <algorithm>
#include <execution>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<int> v(10);
    std::cout << "before for_each(seq)" << std::endl;

    // 即使是 seq，元素访问函数抛出的异常也不会回到这里。
    std::for_each(std::execution::seq, v.begin(), v.end(), [](int) {
        throw std::runtime_error("boom");
    });

    std::cout << "after for_each(seq)" << std::endl;
}
