// 第 29.4 节：同一个 int 放进 vector/list/set/unordered_set，逐元素开销差多少
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

struct Mem {
    static inline std::size_t bytes = 0;
    static inline std::size_t blocks = 0;
    static void reset() { bytes = 0; blocks = 0; }
};

// 一个只做记账的分配器：容器每次申请多少字节，就累加多少。
template <class T>
struct CountingAlloc {
    using value_type = T;

    CountingAlloc() = default;
    template <class U>
    CountingAlloc(const CountingAlloc<U>&) noexcept {}

    T* allocate(std::size_t n) {
        Mem::bytes += n * sizeof(T);
        ++Mem::blocks;
        return std::allocator<T>{}.allocate(n);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        std::allocator<T>{}.deallocate(p, n);
    }

    template <class U>
    bool operator==(const CountingAlloc<U>&) const noexcept { return true; }
    template <class U>
    bool operator!=(const CountingAlloc<U>&) const noexcept { return false; }
};

constexpr int n = 1000;

template <class Container, class Fill>
void measure(const char* name, Fill fill) {
    Mem::reset();
    {
        Container c;
        fill(c);
        std::cout << name << ": " << Mem::bytes << " 字节, "
                  << Mem::blocks << " 次分配, "
                  << static_cast<double>(Mem::bytes) / n << " 字节/元素\n";
    }
}

int main() {
    measure<std::vector<int, CountingAlloc<int>>>("vector   ", [](auto& c) {
        c.reserve(n);                       // 只比较元素本身，不把容量余量算进来
        for (int i = 0; i < n; ++i) c.push_back(i);
    });

    measure<std::deque<int, CountingAlloc<int>>>("deque    ", [](auto& c) {
        for (int i = 0; i < n; ++i) c.push_back(i);
    });

    measure<std::list<int, CountingAlloc<int>>>("list     ", [](auto& c) {
        for (int i = 0; i < n; ++i) c.push_back(i);
    });

    measure<std::set<int, std::less<int>, CountingAlloc<int>>>("set      ", [](auto& c) {
        for (int i = 0; i < n; ++i) c.insert(i);
    });

    measure<std::unordered_set<int, std::hash<int>, std::equal_to<int>,
                               CountingAlloc<int>>>("unordered", [](auto& c) {
        c.reserve(n);                       // 只算桶数组和节点，不算 rehash 的临时开销
        for (int i = 0; i < n; ++i) c.insert(i);
    });
}
