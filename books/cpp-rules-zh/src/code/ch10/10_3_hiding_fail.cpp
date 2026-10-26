// 第 10.3 节：类里出现 operator new 后，全局的其它 new 形式不会被考虑
// mode: fail
// expect: no matching function for call to 'operator new
#include <cstddef>
#include <new>

class Ledger {
public:
    // 只写了一个带额外参数的 placement 版本
    static void* operator new(std::size_t size, void* where) noexcept;
};

class Base {
public:
    static void* operator new(std::size_t size, int tag);
};

class Derived : public Base {
    // 自己没有 operator new，但基类的版本仍然会挡住全局版本
};

class Batch {
public:
    // 数组形式是另一个名字，单独挡住全局 operator new[]
    static void* operator new[](std::size_t size, void* where) noexcept;
};

int main() {
    Ledger* a = new Ledger;                  // 错误：类作用域里没有单参数版本
    Ledger* b = new (std::nothrow) Ledger;   // 错误：nothrow 版本也被挡住
    Derived* c = new Derived;                // 错误：基类的版本挡住了全局版本
    Batch* d = new Batch[2];                 // 错误：类级 operator new[] 挡住了全局版本
    (void)a;
    (void)b;
    (void)c;
    (void)d;
}
