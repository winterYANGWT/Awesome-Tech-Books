// 第 17.4 节：基类重载被隐藏后，调用点直接找不到函数
// mode: fail
// expect: could not bind
#include <string>

class Base {
public:
    virtual ~Base() = default;
    virtual void send(int id) const { (void)id; }
    virtual void send(double ratio) const { (void)ratio; }
};

class Derived : public Base {
public:
    void send(const std::string& text) const { (void)text; }
};

int main() {
    Derived d;
    d.send(7);  // Derived 作用域里只有 send(string)，Base::send(int) 不可见
}
