// 第 7.4 节：临时对象不是 const，可以绑定 T&&，也可以调用非 const 成员
#include <iostream>
#include <type_traits>
#include <utility>

struct Gauge {
    int value = 0;

    void bump() & {
        ++value;
        std::cout << "左值 bump -> " << value << "\n";
    }

    void bump() && {
        ++value;
        std::cout << "右值 bump -> " << value << "\n";
    }

    static Gauge make(int start) {
        Gauge gauge;
        gauge.value = start;
        return gauge;
    }
};

int main() {
    // decltype 看到的是 Gauge，不是 const Gauge。
    static_assert(std::is_same_v<decltype(Gauge::make(30)), Gauge>);

    Gauge gauge;
    gauge.bump();                 // 左值：调用 & 版本

    Gauge::make(10).bump();       // 纯右值：调用 && 版本
    std::move(gauge).bump();      // 将亡值：调用 && 版本

    Gauge&& bound = Gauge::make(20);  // 右值引用延长临时对象的寿命
    bound.bump();                 // bound 本身是左值表达式：调用 & 版本

    std::cout << "gauge.value = " << gauge.value << "\n";
}
