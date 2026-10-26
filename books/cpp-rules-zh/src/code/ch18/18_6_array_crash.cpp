// 第 18.6 节：通过基类指针访问派生数组的第二个元素
// mode: crash
#include <iostream>

class Base {
public:
    virtual ~Base() = default;
    virtual int id() const { return 1; }
};

class Derived : public Base {
public:
    int id() const override { return 2; }
    int payloadFirst() const { return payload_[0]; }

private:
    int payload_[4] = {10, 20, 30, 40};
};

int main() {
    auto* array = new Derived[3];
    Base* base = array;
    std::cout << "array[0].payloadFirst()=" << array[0].payloadFirst() << '\n';

    // base[1] 等价于 *(base + 1)，偏移是 sizeof(Base)，落在第一个 Derived 的 payload_ 里。
    std::cout << "base[1].id()=" << base[1].id() << '\n';

    delete[] base;  // 通过基类指针 delete[] 派生数组也是未定义行为
}
