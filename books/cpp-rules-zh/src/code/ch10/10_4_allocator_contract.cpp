// 第 10.4 节：allocate(n) 的 n 是对象个数，返回未构造的内存
#include <cstddef>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <vector>

struct Stats {
    inline static std::size_t calls = 0;
    inline static std::size_t objects = 0;
    inline static std::size_t bytes = 0;
    inline static std::size_t last_n = 0;
    inline static std::size_t last_size = 0;

    static void reset() {
        calls = objects = bytes = last_n = last_size = 0;
    }
};

template <class T>
struct TrackingAllocator {
    using value_type = T;

    TrackingAllocator() = default;
    template <class U>
    TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        ++Stats::calls;
        Stats::objects += n;
        Stats::bytes += n * sizeof(T);
        Stats::last_n = n;
        Stats::last_size = sizeof(T);
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p); }

    template <class U>
    bool operator==(const TrackingAllocator<U>&) const noexcept { return true; }
    template <class U>
    bool operator!=(const TrackingAllocator<U>&) const noexcept { return false; }
};

struct Tracker {
    inline static int constructed = 0;
    inline static int destroyed = 0;

    Tracker() { ++constructed; }
    Tracker(const Tracker&) { ++constructed; }
    ~Tracker() { ++destroyed; }
};

int main() {
    std::cout << "1) vector::reserve 只调用 allocate，不构造元素\n";
    {
        Stats::reset();
        std::vector<Tracker, TrackingAllocator<Tracker>> v;
        v.reserve(3);
        std::cout << "  allocate 调用 " << Stats::calls << " 次，n=" << Stats::last_n
                  << "，sizeof(Tracker)=" << Stats::last_size
                  << "，总字节 " << Stats::bytes << "\n";
        std::cout << "  reserve 后已构造：" << Tracker::constructed << "\n";
        v.emplace_back();
        v.emplace_back();
        std::cout << "  emplace 两次后已构造：" << Tracker::constructed << "\n";
        v.pop_back();
        std::cout << "  pop_back 后已析构：" << Tracker::destroyed << "\n";
    }
    std::cout << "  离开作用域后已析构：" << Tracker::destroyed << "\n";

    std::cout << "2) list<int> 的 allocator<int> 被 rebind 成节点分配器\n";
    Stats::reset();
    {
        std::list<int, TrackingAllocator<int>> values;
        values.push_back(1);
        values.push_back(2);
        values.push_back(3);
        std::cout << "  allocate 调用 " << Stats::calls << " 次，每次 n=" << Stats::last_n
                  << "，单块 " << Stats::last_size << " 字节，总字节 " << Stats::bytes << "\n";
        std::cout << "  （3 个元素 3 个节点；本机 libstdc++ 把哨兵嵌在 list 对象里，不单独分配）\n";
    }
}
