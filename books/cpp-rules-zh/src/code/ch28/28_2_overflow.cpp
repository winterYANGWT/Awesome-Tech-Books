// 第 28.2 节：C API 写超过 vector 的 size/capacity，ASan 当场抓住
// mode: crash
// expect: heap-buffer-overflow
#include <cstddef>
#include <iostream>
#include <vector>

// 这个遗留接口没有长度参数，固定写三个 int。
extern "C" void fill_three(int* out) {
    out[0] = 1;
    out[1] = 2;
    out[2] = 3;
}

int main() {
    std::vector<int> values(1);  // size 和 capacity 都只有 1
    fill_three(values.data());
    std::cout << "写完了，size 仍然是 " << values.size() << '\n';
}
