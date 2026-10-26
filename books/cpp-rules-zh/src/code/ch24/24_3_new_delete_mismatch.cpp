// 第 24.3 节：隔着函数，编译器看不到 new[]/delete 的错配，运行期才暴露
// mode: crash
// expect: alloc-dealloc-mismatch
#include <iostream>
#include <numeric>

// 把释放藏进另一个函数：直接写在 main 里编译器会警告，隔一层就不报了
void release(int* p) {
    delete p;  // 错：数组必须用 delete[] 释放
}

int main() {
    int* samples = new int[4]{3, 1, 4, 1};
    std::cout << "和: " << std::accumulate(samples, samples + 4, 0) << '\n';

    release(samples);
    std::cout << "这一行在正确的程序里不应该出现\n";
}
