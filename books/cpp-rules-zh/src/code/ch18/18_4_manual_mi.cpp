// 第 18.4 节：手工模拟多继承，指针转换、RTTI 和回指指针都要自己补
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class B1 {
public:
    virtual ~B1() = default;
    virtual std::string name() const { return "B1"; }
};

class B2 {
public:
    virtual ~B2() = default;
    virtual std::string name() const { return "B2"; }
};

// 把 dynamic_cast 放进函数，让编译器只看到 B1*，看不到具体对象。
B2* castToB2(B1* object) { return dynamic_cast<B2*>(object); }

// 不用多继承：公开继承 B1，内含一个继承 B2 的嵌套对象。
class D : public B1 {
public:
    class D2 : public B2 {
    public:
        explicit D2(D* owner) : owner_(owner) {}
        std::string name() const override { return owner_->name(); }

    private:
        friend class D;
        D* owner_;
    };

    D() : d2_(this) {}
    D(const D& other) : B1(other), tag_(other.tag_), d2_(this) {}
    D& operator=(const D& other) {
        B1::operator=(other);
        tag_ = other.tag_;
        d2_.owner_ = this;
        return *this;
    }

    operator B2&() { return d2_; }
    B2& asB2() { return d2_; }

    std::string name() const override { return tag_; }
    void rename(std::string tag) { tag_ = std::move(tag); }

private:
    std::string tag_ = "D";
    D2 d2_;
};

int main() {
    D d;
    B2& reference = d;  // 靠 operator B2&
    std::cout << "D& -> B2& 可用：" << reference.name() << '\n';

    B2* pointer = &d.asB2();  // 指针没有自动转换
    std::cout << "D* -> B2* 需要 asB2()：" << pointer->name() << '\n';

    B2* fromRtti = castToB2(&d);  // D2 不是 D 的基类，RTTI 也看不到这条边
    std::cout << std::boolalpha << "castToB2(&d) 是 nullptr："
              << (fromRtti == nullptr) << '\n';

    d.rename("original");
    D copy = d;
    copy.rename("copy");
    std::cout << "复制后修正回指：" << copy.asB2().name() << '\n';

    static_assert(std::is_convertible_v<D&, B2&>);
    static_assert(!std::is_convertible_v<D*, B2*>);
}
