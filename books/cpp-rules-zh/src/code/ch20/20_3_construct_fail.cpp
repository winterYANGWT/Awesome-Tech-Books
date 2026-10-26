// 第 20.3 节：单参数 construct 让目标类型和初值类型互相冲突
// mode: fail
#include <new>

template<class T1>
void construct(T1* p, const T1& value) {
    ::new (static_cast<void*>(p)) T1(value);
}

struct Base {};
struct Derived : Base {};

int main() {
    double d = 0;
    construct(&d, 42);  // T1 是 double 还是 int？

    Base* pb = nullptr;
    Derived der;
    construct(pb, der);  // T1 是 Base 还是 Derived？
}
