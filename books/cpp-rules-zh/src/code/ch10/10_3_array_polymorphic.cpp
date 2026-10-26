// 第 10.3 节：通过基类指针 delete[] 派生类数组是未定义行为
// mode: crash
// expect: new-delete-type-mismatch
#include <iostream>

struct Base {
    virtual ~Base() = default;
    int base_value = 1;
};

struct Derived : Base {
    int payload[4]{};
};

int main() {
    Base* p = new Derived[3];
    std::cout << "通过 Base* delete[] Derived[3]\n";
    delete[] p;
    std::cout << "不应执行到这里\n";
}
