// 第 19.1 节：虚函数给每个对象加一个 vptr，并让跨类型调用变成间接调用
#include <iostream>
#include <string>

struct Plain {
    int id = 0;
};

class Poly {
public:
    virtual ~Poly() = default;
    virtual std::string name() const { return "Poly"; }
    int id = 0;
};

class Named : public Poly {
public:
    std::string name() const override { return "Named"; }
};

// 两个互不相关的多态基类：每个基类子对象都要有自己的 vptr。
struct Left {
    virtual ~Left() = default;
    int left = 1;
};

struct Right {
    virtual ~Right() = default;
    int right = 2;
};

struct Both : Left, Right {
    int both = 3;
};

void describe(const Poly& p) {
    // 编译期只知道 p 是 Poly&，目标函数要到运行期查表。
    std::cout << "通过 Poly& 调用: " << p.name() << '\n';
}

int main() {
    std::cout << "sizeof(Plain)=" << sizeof(Plain) << '\n';
    std::cout << "sizeof(Poly)=" << sizeof(Poly) << '\n';
    std::cout << "sizeof(Both)=" << sizeof(Both) << '\n';

    Named named;
    describe(named);

    Both both;
    const Left* left = &both;
    const Right* right = &both;
    // 多重继承下，不同基类子对象在对象里的偏移不同，this 指针要相应调整。
    std::cout << "Left 与 Right 子对象地址相同? "
              << (static_cast<const void*>(left) == static_cast<const void*>(right)) << '\n';
    std::cout << "sizeof(Left)=" << sizeof(Left)
              << " sizeof(Right)=" << sizeof(Right) << '\n';
}
