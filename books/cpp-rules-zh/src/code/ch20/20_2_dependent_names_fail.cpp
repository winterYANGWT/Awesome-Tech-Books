// 第 20.2 节：依赖名默认不是类型，模板基类的名字也不会被自动查找
// mode: fail
#include <vector>

// 错误一：变量声明里的 C::const_iterator 没写 typename
template<class C>
void print_second(const C& c) {
    C::const_iterator it = c.begin();
    ++it;
}

// 错误二：Derived 不会到依赖的 Base<T> 里找 base_op
template<class T>
struct Base {
    void base_op() {}
};

template<class T>
struct Derived : Base<T> {
    void call() { base_op(); }
};

int main() {
    std::vector<int> numbers{1, 2};
    print_second(numbers);
    Derived<int> d;
    d.call();
}
