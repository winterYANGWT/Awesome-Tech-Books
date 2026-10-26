// 第 5.4 节：通用引用、右值引用，以及形参本身永远是左值
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct tag {
    using type = T;
};

template <typename T>
tag<T> const_rref(const T&&);          // const T&&：普通右值引用

template <typename T>
tag<T> vector_rref(std::vector<T>&&);  // vector<T>&&：普通右值引用

template <typename T>
void observe(T&& param) {
    std::cout << (std::is_lvalue_reference_v<T> ? "左值实参" : "右值实参")
              << "：T 是引用 = " << std::is_reference_v<T>
              << "，T 去掉引用后是 const = "
              << std::is_const_v<std::remove_reference_t<T>>
              << "，param 是左值表达式 = "
              << std::is_lvalue_reference_v<decltype((param))> << "\n";
}

template <typename T>
struct Box {
    // T 在 Box 实例化时已经确定，store 调用时不再推导，所以是右值引用
    void store(T&& value) { value_ = value; }

    // A 是独立的模板参数，每次调用都推导，所以是通用引用
    template <typename A>
    void store_any(A&& value) { value_ = value; }

    const T& value() const { return value_; }

private:
    T value_{};
};

int main() {
    int x = 1;
    const int cx = 2;

    static_assert(std::is_same_v<decltype(const_rref(42))::type, int>);
    static_assert(std::is_same_v<decltype(vector_rref(std::vector<int>{1}))::type, int>);

    // auto&& 与函数模板的 T&& 同理
    auto&& u1 = x;      // int&
    auto&& u2 = cx;     // const int&
    auto&& u3 = 3;      // int&&
    static_assert(std::is_same_v<decltype(u1), int&>);
    static_assert(std::is_same_v<decltype(u2), const int&>);
    static_assert(std::is_same_v<decltype(u3), int&&>);

    observe(x);
    observe(cx);
    observe(4);

    Box<std::string> box;
    box.store(std::string("右值进入 store"));
    std::cout << "store 之后：" << box.value() << "\n";

    std::string local = "左值进入 store_any";
    box.store_any(local);
    std::cout << "store_any 之后：" << box.value()
              << "，local = " << local << "\n";

    // 传右值时 store_any 也接受，且 local 没有被搬走
    box.store_any(std::string("右值进入 store_any"));
    std::cout << "再次 store_any 之后：" << box.value() << "\n";
}
