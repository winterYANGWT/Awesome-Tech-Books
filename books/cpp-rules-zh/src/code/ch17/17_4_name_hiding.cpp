// 第 17.4 节：派生类的同名声明会隐藏基类全部重载
// flags: -Wno-overloaded-virtual
#include <iostream>
#include <string>

class Base {
public:
    virtual ~Base() = default;
    virtual void send(int id) const { std::cout << "Base::send(int " << id << ")\n"; }
    virtual void send(double ratio) const { std::cout << "Base::send(double " << ratio << ")\n"; }
    virtual void send(const std::string& text) const {
        std::cout << "Base::send(string " << text << ")\n";
    }
};

class Hiding : public Base {
public:
    // 只声明一个同名函数，Base::send 的另外两个重载全部被隐藏。
    void send(const std::string& text) const override {
        std::cout << "Hiding::send(string " << text << ")\n";
    }
};

// 如果派生类只想要一个字符参数，int 会悄悄转成 char：
// 调用点看起来像在调用 Base::send(int)，实际进了派生类的新函数。
// clang 的 -Woverloaded-virtual（-Wall 已包含）会警告这种隐藏；
// 这里关掉警告，让程序跑起来，把调用结果展示出来。
class Narrow : public Base {
public:
    void send(char code) const {
        std::cout << "Narrow::send(char " << code << ")\n";
    }
};

class Exposing : public Base {
public:
    using Base::send;  // 把整组同名重载重新引入派生类作用域
    void send(const std::string& text) const override {
        std::cout << "Exposing::send(string " << text << ")\n";
    }
};

int main() {
    Hiding hiding;
    hiding.send("hi");
    Base& hidingBase = hiding;
    hidingBase.send(7);       // 静态类型是 Base&，看得到 send(int)
    hidingBase.send("hi");    // 虚调用仍然分派到 Hiding::send

    Narrow narrow;
    narrow.send(65);          // 没有编译错误，65 被转成 char 'A'
    Base& narrowBase = narrow;
    narrowBase.send(65);      // 通过 Base& 才调用 Base::send(int)

    Exposing exposing;
    exposing.send(7);         // using 之后重载重新可见
    exposing.send("hi");
    Base& exposingBase = exposing;
    exposingBase.send(2.5);
}
