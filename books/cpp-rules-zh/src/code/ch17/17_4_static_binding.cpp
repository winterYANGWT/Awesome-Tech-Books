// 第 17.4 节：非虚函数和默认实参都按静态类型绑定
#include <iostream>

class Store {
public:
    virtual ~Store() = default;

    // 非虚：Store 承诺所有派生类都用同一套描述方式。
    void describe() const { std::cout << "Store::describe（非虚）\n"; }

    // 虚函数 + 默认实参：函数体动态绑定，默认值静态绑定。
    virtual void open(int mode = 1) const {
        std::cout << "Store::open(" << mode << ")\n";
    }
};

class FileStore : public Store {
public:
    // 重定义继承来的非虚函数：这不是覆盖，只是隐藏。
    void describe() const { std::cout << "FileStore::describe（非虚）\n"; }

    void open(int mode = 2) const override {
        std::cout << "FileStore::open(" << mode << ")\n";
    }
};

int main() {
    FileStore file;
    Store* base = &file;
    FileStore* derived = &file;

    base->describe();     // 静态类型 Store*，调用 Store::describe
    derived->describe();  // 静态类型 FileStore*，调用 FileStore::describe

    base->open();         // 函数体是 FileStore::open，默认实参却来自 Store
    derived->open();      // 默认实参来自 FileStore

    const Store plain;
    plain.open();
}
