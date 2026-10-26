// 第 19.4 节：typeid 作用在指针上得到静态类型，解引用才查动态类型
#include <iostream>
#include <string>
#include <typeinfo>

class Base {
public:
    virtual ~Base() = default;
    virtual std::string name() const { return "Base"; }
};

class Alpha : public Base {
public:
    std::string name() const override { return "Alpha"; }
};

class Beta : public Base {
public:
    std::string name() const override { return "Beta"; }
};

class Gamma : public Alpha {
public:
    std::string name() const override { return "Gamma"; }
};

struct Plain {
    int value = 0;
};

void inspect(Base* p) {
    std::cout << "  name(): " << p->name() << '\n';
    // p 的静态类型是 Base*，所以这里永远是 Base*，和对象实际类型无关。
    std::cout << "  typeid(p) == typeid(Base*): "
              << (typeid(p) == typeid(Base*)) << '\n';
    // *p 是多态类型的左值，typeid 才会查动态类型。
    std::cout << "  typeid(*p) == typeid(Alpha): "
              << (typeid(*p) == typeid(Alpha)) << '\n';
    std::cout << "  typeid(*p) == typeid(Beta): "
              << (typeid(*p) == typeid(Beta)) << '\n';
    if (dynamic_cast<Alpha*>(p) != nullptr) {
        std::cout << "  dynamic_cast<Alpha*>: ok\n";
    } else {
        std::cout << "  dynamic_cast<Alpha*>: null\n";
    }
}

int main() {
    Alpha alpha;
    Beta beta;
    std::cout << "Alpha 对象：\n";
    inspect(&alpha);
    std::cout << "Beta 对象：\n";
    inspect(&beta);
    Gamma gamma;
    std::cout << "Gamma 对象（进一步派生自 Alpha）：\n";
    inspect(&gamma);

    Base* null = nullptr;
    try {
        (void)typeid(*null);
        std::cout << "空指针没有抛异常\n";
    } catch (const std::bad_typeid&) {
        // 多态类型的空指针解引用取 typeid，会抛 std::bad_typeid。
        std::cout << "多态空指针: 捕获 bad_typeid\n";
    }

    // 非多态类型不做动态查询，也不求值；空指针同样只得到静态类型。
    Plain plain;
    Plain* plain_ptr = &plain;
    Plain* plain_null = nullptr;
    std::cout << "非多态: typeid(*plain_ptr) == typeid(Plain): "
              << (typeid(*plain_ptr) == typeid(Plain)) << '\n';
    std::cout << "非多态空指针: typeid(*plain_null) == typeid(Plain): "
              << (typeid(*plain_null) == typeid(Plain)) << '\n';
}
