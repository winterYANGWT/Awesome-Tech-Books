// 第 36.5 节：ASan 抓住 vector 的越界读，编译器警告看不见它
// mode: crash
// expect: heap-buffer-overflow
#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    const std::vector<int> quantities{3, 5, 8};
    const int* data = quantities.data();
    int total = 0;
    for (std::size_t i = 0; i <= quantities.size(); ++i) {
        total += data[i];  // i == size() 时越界
    }
    std::cout << "total=" << total << '\n';
}
