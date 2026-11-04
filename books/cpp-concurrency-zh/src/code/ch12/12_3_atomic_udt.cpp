// mode: run
// flags: -latomic
#include <atomic>
#include <cstddef>
#include <iostream>

// 16 字节：一个指针加一个计数器，常被拿来做带标记的指针
struct Pair {
    void* ptr;
    unsigned long tag;
};

int main() {
    std::atomic<Pair> value{Pair{nullptr, 0}};

    std::cout << std::boolalpha;
    std::cout << "sizeof(std::atomic<Pair>) = " << sizeof(value) << '\n';
    std::cout << "alignof(std::atomic<Pair>) = " << alignof(std::atomic<Pair>) << '\n';
    std::cout << "is_always_lock_free = " << std::atomic<Pair>::is_always_lock_free << '\n';
    std::cout << "is_lock_free() = " << value.is_lock_free() << '\n';

    Pair expected{nullptr, 0};
    const Pair desired{&value, 1};
    std::cout << "CAS 成功? " << value.compare_exchange_strong(expected, desired) << '\n';

    // 对照（本机需在源文件后链接 -latomic）：
    // clang++ -std=c++20 -O2 -mcx16 12_3_atomic_udt.cpp -latomic
    // GCC 15 加不加 -mcx16 都是 false；Clang 21 加 -mcx16 才是 true
}
