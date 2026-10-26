// 第 20.4 节：函数模板的特化不参与重载决议，先选主模板再找特化
#include <iostream>

template<class T>
void f(T) {
    std::cout << "f(T)\n";
}

template<>
void f<int*>(int*) {
    std::cout << "f<int*>\n";
}

template<class T>
void f(T*) {  // 这是另一个主模板，不是 f(T) 的偏特化
    std::cout << "f(T*)\n";
}

void g(int) {
    std::cout << "g(int) 非模板\n";
}

template<class T>
void g(T) {
    std::cout << "g(T)\n";
}

template<class T>
void h(T) {
    std::cout << "h(T)\n";
}

template<class T>
void h(T*) {
    std::cout << "h(T*)\n";
}

int main() {
    int i = 0;

    f(&i);         // 主模板 f(T*) 更特化，选中它
    f<int*>(&i);   // 显式指定模板实参，选中 f<int*> 特化

    g(1);          // 非模板精确匹配，优先
    g(1.0);        // 非模板要转换，模板胜出

    h(&i);         // h(T*) 与 h(T) 都是主模板，h(T*) 更特化
}
