// 第 16.2 节：前置 ++ 做实际工作，后置 ++ 保存旧值、调用前置、按值返回旧值
#include <iostream>
#include <type_traits>

class Step {
public:
    explicit Step(int value) : value_(value) {}

    Step(const Step& other) : value_(other.value_) { ++copies_; }
    Step& operator=(const Step&) = default;

    // 前置：先加，再返回自身引用，没有临时对象
    Step& operator++() {
        ++value_;
        return *this;
    }

    // 后置：保存旧值，调用前置，再返回旧值。
    // 末尾的 & 限定它只能作用于左值，从语言层面禁掉 s++++。
    Step operator++(int) & {
        Step old = *this;      // 至少一次复制，旧值必须留下来
        ++*this;               // 增量逻辑只写在前置版本里
        return old;
    }

    Step& operator+=(int amount) {
        value_ += amount;
        return *this;
    }

    int value() const { return value_; }
    static int copies() { return copies_; }

private:
    int value_;
    static inline int copies_ = 0;
};

int main() {
    Step s(10);

    int before = Step::copies();
    Step old = s++;            // 后置：拿到 10，s 变成 11
    int after_postfix = Step::copies();
    std::cout << "s++ 得到 " << old.value() << "，s 现在是 " << s.value() << '\n';

    Step& current = ++s;       // 前置：s 变成 12，返回引用
    int after_prefix = Step::copies();
    std::cout << "++s 之后 s 是 " << current.value() << '\n';
    std::cout << "后置增加复制 " << after_postfix - before
              << " 次，前置增加复制 " << after_prefix - after_postfix << " 次\n";

    // 复合赋值返回引用，可以继续往左结合，且不产生临时对象
    (s += 3) += 4;
    std::cout << "链式 += 之后 s 是 " << s.value() << '\n';

    static_assert(std::is_same_v<decltype(++s), Step&>);
    static_assert(std::is_same_v<decltype(s++), Step>);
}
