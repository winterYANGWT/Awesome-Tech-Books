// 第 5.3 节：typeid 会丢掉引用和顶层 const，static_assert 不会
#include <iostream>
#include <type_traits>
#include <typeinfo>

struct Gauge {
    int value = 42;
};

template <typename T>
void inspect(const T& param) {
    // param 的声明类型是 const T&；name() 按“按值传参”处理，丢掉引用和顶层 const
    std::cout << "T      = " << typeid(T).name() << "\n";
    std::cout << "param  = " << typeid(param).name() << "\n";
    // 类型本身是 const T&，即 const Gauge* const&；typeid 却把它和 T 看成同一个
    static_assert(std::is_same_v<decltype(param), const Gauge* const&>);
    std::cout << std::boolalpha
              << "typeid(param) == typeid(const Gauge*): "
              << (typeid(param) == typeid(const Gauge*)) << "\n"
              << "typeid(param) == typeid(T):            "
              << (typeid(param) == typeid(T)) << "\n";
}

int main() {
    Gauge gauge;
    const Gauge* pointer = &gauge;

    inspect(pointer);

    // 编译期检查才是准的：T 保留了指向 const 的指针，param 又加了一层 const&
    static_assert(std::is_same_v<decltype(pointer), const Gauge*>);
}
