// 第 3.5 节：按字节复制 std::string 会得到两个拥有同一缓冲区的对象
// mode: crash
// expect: double-free
#include <cstdio>
#include <cstring>
#include <string>

// 把 memcpy 藏进一个通用函数，调用点就看不出搬的是什么类型
void raw_copy(void* destination, const void* source, std::size_t size) {
    std::memcpy(destination, source, size);
}

int main() {
    std::string source = "a string long enough to avoid SSO";
    std::string target = "another string long enough too!!";

    raw_copy(&target, &source, sizeof target);
    std::printf("target looks like: %s\n", target.c_str());
    std::printf("two objects now own one buffer\n");
}
