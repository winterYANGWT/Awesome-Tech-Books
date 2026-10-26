// 第 20.4 节：把定制点放进类模板，函数模板只负责转调
#include <iostream>

template<class T>
struct TypeInfo {
    static constexpr const char* name = "unknown";
};

// 全特化
template<>
struct TypeInfo<bool> {
    static constexpr const char* name = "bool";
};

// 偏特化：只固定“是指针”这一部分
template<class T>
struct TypeInfo<T*> {
    static constexpr const char* name = "pointer";
};

// 第三方类型也能定制，不用修改 Money 本身
struct Money {
    long cents;
};

template<>
struct TypeInfo<Money> {
    static constexpr const char* name = "Money";
};

// 函数模板永不特化，只把工作交给可定制的类模板
template<class T>
const char* type_name() {
    return TypeInfo<T>::name;
}

int main() {
    std::cout << "double: " << type_name<double>() << '\n';
    std::cout << "bool: " << type_name<bool>() << '\n';
    std::cout << "int*: " << type_name<int*>() << '\n';
    std::cout << "Money: " << type_name<Money>() << '\n';
}
