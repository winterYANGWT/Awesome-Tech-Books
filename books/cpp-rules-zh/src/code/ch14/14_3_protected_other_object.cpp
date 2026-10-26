// 第 14.3 节：派生类不能借 Base* 访问另一个对象的 protected 成员
// mode: fail
// expect: protected
class Base {
protected:
    int secret_ = 7;

public:
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    int peek(const Base& other) const { return other.secret_; }
};

int main() {
    Derived d;
    return d.peek(d);   // 实际对象是 Derived，静态类型仍是 Base&
}
