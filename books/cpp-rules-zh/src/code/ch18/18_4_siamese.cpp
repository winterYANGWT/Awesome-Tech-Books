// 第 18.4 节：两个基类有同名虚函数时，一个 override 会同时覆盖两边
#include <iostream>
#include <string>

class BaseA {
public:
    virtual ~BaseA() = default;
    virtual std::string readBuf(const std::string& text) const { return "A:" + text; }
};

class BaseB {
public:
    virtual ~BaseB() = default;
    virtual std::string readBuf(const std::string& text) const { return "B:" + text; }
};

// 一个 readBuf 同时覆盖 BaseA::readBuf 和 BaseB::readBuf。
class Naive : public BaseA, public BaseB {
public:
    std::string readBuf(const std::string& text) const override {
        return "same:" + text;
    }
};

// 中间类把旧名字转发到新名字，两个接口就分开了。
class BaseA2 : public BaseA {
public:
    virtual std::string baseAReadBuf(const std::string& text) const = 0;

private:
    std::string readBuf(const std::string& text) const override {
        return baseAReadBuf(text);
    }
};

class BaseB2 : public BaseB {
public:
    virtual std::string baseBReadBuf(const std::string& text) const = 0;

private:
    std::string readBuf(const std::string& text) const override {
        return baseBReadBuf(text);
    }
};

class Separated : public BaseA2, public BaseB2 {
public:
    std::string baseAReadBuf(const std::string& text) const override {
        return "A:" + text;
    }
    std::string baseBReadBuf(const std::string& text) const override {
        return "B:" + text;
    }
};

int main() {
    Naive naive;
    const BaseA& a = naive;
    const BaseB& b = naive;
    std::cout << "连体：A 接口 -> " << a.readBuf("x")
              << "；B 接口 -> " << b.readBuf("x") << '\n';

    Separated separated;
    const BaseA& a2 = separated;
    const BaseB& b2 = separated;
    std::cout << "拆开：A 接口 -> " << a2.readBuf("x")
              << "；B 接口 -> " << b2.readBuf("x") << '\n';

    static_assert(std::is_abstract_v<BaseA2>);
    static_assert(std::is_abstract_v<BaseB2>);
}
