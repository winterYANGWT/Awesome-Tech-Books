// 第 6.6 节：构造抛出时，已经显式析构的自动对象会被再次析构
// mode: crash
// expect: double-free
#include <iostream>
#include <new>
#include <stdexcept>

struct Resource {
    int* data;

    Resource() : data(new int(42)) {}

    // 在成员初始化之前就抛出：旧对象留下的 data 指针没有被覆盖。
    explicit Resource(int) {
        throw std::runtime_error("construction failed");
    }

    ~Resource() { delete data; }
};

void rebuild() {
    Resource t;             // 正常构造，data 指向一块堆内存
    t.~Resource();          // 显式析构，delete 掉 data
    new (&t) Resource(1);   // 抛出：新对象没有构造完成，t 的生命周期没有重新开始
}                           // 自动变量 t 离开作用域，析构函数被再次调用

int main() {
    try {
        rebuild();
    } catch (const std::exception& e) {
        std::cout << "捕获: " << e.what() << '\n';
    }
}
