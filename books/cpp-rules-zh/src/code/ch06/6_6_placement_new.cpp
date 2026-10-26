// 第 6.6 节：析构后在同址 placement new 合法；现代写法用 optional
#include <iostream>
#include <new>
#include <optional>
#include <string>
#include <utility>

struct Widget {
    static inline int constructions = 0;
    static inline int destructions = 0;

    Widget(std::string name, int generation)
        : name(std::move(name)), generation(generation) {
        ++constructions;
    }
    ~Widget() { ++destructions; }

    std::string name;
    int generation = 0;
};

int main() {
    {
        Widget t("old", 1);
        Widget& rt = t;       // 引用绑定的是存储位置，不是旧对象
        t.~Widget();          // 显式结束生命周期
        std::cout << "析构后: constructions=" << Widget::constructions
                  << " destructions=" << Widget::destructions << '\n';

        new (&t) Widget("new", 2);   // 在同一块存储上构造新对象
        std::cout << "重建后: t=" << t.name << '/' << t.generation
                  << " rt=" << rt.name << '/' << rt.generation << '\n';
        std::cout << "重建后: constructions=" << Widget::constructions
                  << " destructions=" << Widget::destructions << '\n';
    }   // 新对象在这里正常析构一次
    std::cout << "离开作用域: constructions=" << Widget::constructions
              << " destructions=" << Widget::destructions << '\n';

    // 现代替代：optional 明确表达“可能没有值”，emplace 负责销毁旧值再构造。
    std::optional<Widget> slot;
    slot.emplace("first", 1);
    slot.emplace("second", 2);   // 先析构 first，再构造 second
    std::cout << "optional: has_value=" << slot.has_value()
              << " name=" << slot->name
              << " constructions=" << Widget::constructions
              << " destructions=" << Widget::destructions << '\n';
    slot.reset();
    std::cout << "optional reset: has_value=" << slot.has_value()
              << " destructions=" << Widget::destructions << '\n';
}
