// 第 35.2 节：new 出来的内存用 free 释放，分配器不匹配
// mode: crash
// expect: alloc-dealloc-mismatch
#include <cstdlib>
#include <iostream>

int main() {
    int* reading = new int(42);
    std::cout << "reading=" << *reading << '\n';
    std::free(reading);  // 必须配 delete；这里故意配错，让 ASan 抓住
}
