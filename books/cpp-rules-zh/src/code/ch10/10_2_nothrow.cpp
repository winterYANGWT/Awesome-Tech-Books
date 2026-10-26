// 第 10.2 节：nothrow new 只保证分配函数不抛，构造函数仍可能抛
// sanitize: none
#include <cstddef>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>

class Service {
public:
    Service() { throw std::runtime_error("服务启动失败"); }
};

int main() {
    std::cout << "1) 分配成功、构造函数抛异常：异常照常传播\n";
    try {
        Service* s = new (std::nothrow) Service;
        std::cout << "  构造成功 " << static_cast<void*>(s) << "\n";
    } catch (const std::exception& e) {
        std::cout << "  捕获：" << e.what() << "\n";
    }

    std::cout << "2) 分配失败：nothrow 版本返回空指针，不抛 bad_alloc\n";
    void* huge = ::operator new(std::numeric_limits<std::size_t>::max() / 2, std::nothrow);
    std::cout << "  huge 是空指针：" << (huge == nullptr ? "是" : "否") << "\n";
    if (huge != nullptr) ::operator delete(huge);

    std::cout << "3) 小对象正常分配\n";
    int* n = new (std::nothrow) int(42);
    std::cout << "  n 非空：" << (n != nullptr ? "是" : "否") << "，值 " << *n << "\n";
    delete n;
}
