// 第 10.6 节：用计数基类限制一个类的对象数量，用函数内静态对象做单例
#include <iostream>

template <class BeingCounted>
class Counted {
public:
    class TooManyObjects {};

    static int objectCount() { return numObjects_; }

protected:
    Counted() { init(); }
    Counted(const Counted&) { init(); }
    ~Counted() { --numObjects_; }

private:
    static void init() {
        if (numObjects_ >= BeingCounted::maxObjects) throw TooManyObjects{};
        ++numObjects_;
    }

    inline static int numObjects_ = 0;
};

class Printer : private Counted<Printer> {
public:
    static constexpr int maxObjects = 2;

    Printer() = default;
    ~Printer() = default;

    using Counted<Printer>::objectCount;
    using Counted<Printer>::TooManyObjects;
};

class Settings {
public:
    static Settings& instance() {
        static Settings only;   // C++11 起线程安全的局部静态
        return only;
    }
    int value() const { return 42; }

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

private:
    Settings() = default;
};

int main() {
    std::cout << "1) 至多两个 Printer\n";
    std::cout << "  初始对象数：" << Printer::objectCount() << "\n";
    Printer* a = new Printer;
    Printer* b = new Printer;
    std::cout << "  创建两个后：" << Printer::objectCount() << "\n";
    try {
        Printer third;   // 栈对象也走同一个计数基类
        (void)third;
    } catch (const Printer::TooManyObjects&) {
        std::cout << "  第三个被拒绝：TooManyObjects\n";
    }
    delete a;
    delete b;
    std::cout << "  删除两个后：" << Printer::objectCount() << "\n";

    std::cout << "2) 单例：函数内静态对象只构造一次\n";
    Settings& s1 = Settings::instance();
    Settings& s2 = Settings::instance();
    std::cout << "  两次取到同一对象：" << (&s1 == &s2 ? "是" : "否")
              << "，值 " << s1.value() << "\n";
}
