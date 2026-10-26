// 第 20.1 节：swap 的要求不只是能赋值，还要说清异常时保证什么
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

struct Ledger {
    int balance;

    explicit Ledger(int b) : balance(b) {}
    Ledger(const Ledger&) = default;

    Ledger& operator=(const Ledger& other) {
        balance = other.balance;          // 先改
        if (--remaining == 0) {
            balance = -1;                 // 再抛：连对方原来的值也没了
            throw std::runtime_error("ledger write failed");
        }
        return *this;
    }

    static int remaining;
};

int Ledger::remaining = 2;  // 让第二次赋值失败

// 教科书写法：拷贝构造 + 两次拷贝赋值，没有额外承诺
template<class T>
void brute_swap(T& a, T& b) {
    T temp(a);
    a = b;
    b = temp;
}

struct Buffer {
    std::vector<int> data;

    void swap(Buffer& other) noexcept { data.swap(other.data); }
};

int adl_swaps = 0;

// 类型所在命名空间里的非成员 swap，靠 ADL 找到
void swap(Buffer& a, Buffer& b) noexcept {
    ++adl_swaps;
    a.swap(b);
}

static_assert(!std::is_nothrow_swappable_v<Ledger>);
static_assert(std::is_nothrow_swappable_v<Buffer>);

int main() {
    Ledger a(100), b(200);
    try {
        brute_swap(a, b);
    } catch (const std::exception& e) {
        std::cout << "exception: " << e.what() << '\n';
    }
    std::cout << "after failed swap: a=" << a.balance << " b=" << b.balance << '\n';

    Buffer x{{1, 2, 3}}, y{{9, 8}};
    using std::swap;
    swap(x, y);  // ADL 选中上面的 swap(Buffer&, Buffer&)
    std::cout << "adl swaps = " << adl_swaps << ", x.size = " << x.data.size()
              << ", y.size = " << y.data.size() << '\n';
}
