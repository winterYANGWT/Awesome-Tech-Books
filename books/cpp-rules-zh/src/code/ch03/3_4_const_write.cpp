// 第 3.4 节：对真正的 const 对象去 const 再写入是未定义行为
// mode: crash
// expect: SEGV
#include <cstdio>

const int limit = 10;

int main() {
    int* writable = const_cast<int*>(&limit);
    std::printf("about to write\n");
    *writable = 20;
    std::printf("limit = %d\n", limit);
}
