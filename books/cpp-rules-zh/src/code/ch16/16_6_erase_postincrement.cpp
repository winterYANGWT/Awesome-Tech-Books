// 第 16.6 节：erase(i++) 先完成自增，再把旧迭代器交给 erase
#include <iostream>
#include <list>
#include <stdexcept>

class Tracer {
public:
    explicit Tracer(int value) : value_(value) {}

    Tracer operator++(int) {
        Tracer old = *this;
        ++value_;
        return old;
    }

    int value() const { return value_; }

private:
    int value_;
};

void observe(Tracer tracer) {
    std::cout << "f 看到 " << tracer.value() << '\n';
    throw std::runtime_error("f 失败");
}

int main() {
    std::list<int> values{10, 20, 30, 40};
    for (auto it = values.begin(); it != values.end();) {
        if (*it % 20 == 0) {
            values.erase(it++);     // 先把 it 移到下一个元素，再删除旧位置
        } else {
            ++it;
        }
    }

    std::cout << "删除后:";
    for (int value : values) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    Tracer a(1);
    try {
        observe(a++);               // a 先自增，再把旧值传给 f
    } catch (const std::exception& e) {
        std::cout << "f(a++) 之后 a=" << a.value() << "（" << e.what() << "）\n";
    }

    Tracer b(1);
    try {
        observe(b);                 // f 抛异常，下一句 b++ 不会执行
        b++;
    } catch (const std::exception& e) {
        std::cout << "f(b); b++ 之后 b=" << b.value() << "（" << e.what() << "）\n";
    }
}
