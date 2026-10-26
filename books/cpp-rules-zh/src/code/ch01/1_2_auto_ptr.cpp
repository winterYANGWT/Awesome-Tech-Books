// 第 1.2 节：auto_ptr 在 C++17 已被标准移除；libstdc++ 15 仍保留弃用版本
// mode: warn
// expect: deprecated

#include <cstdio>
#include <memory>

int main() {
    std::auto_ptr<int> owner(new int(7));
    std::printf("%d\n", *owner);
}
