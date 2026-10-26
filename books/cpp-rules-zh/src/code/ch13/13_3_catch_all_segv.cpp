// 第 13.3 节：catch(...) 只接 throw 出来的对象，接不住空指针解引用
// mode: crash
// sanitize: address
// expect: SEGV
#include <iostream>

int main() {
    try {
        throw 42;
    } catch (...) {
        std::cout << "catch(...) 接住了 throw 42\n";
    }

    try {
        const int* value = nullptr;
        std::cout << "准备解引用空指针\n";
        std::cout << *value << "\n";
    } catch (...) {
        std::cout << "这一行不会执行\n";
    }
}
