// 第 24.2 节：查找远多于增删时，排序 vector 往往比 map 更省内存
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct AllocationStats {
    static inline std::size_t bytes = 0;
    static inline std::size_t blocks = 0;
    static void reset() { bytes = 0; blocks = 0; }
};

// 记录容器真正向堆要了多少字节。map 的节点类型由实现决定，所以这里不猜
// sizeof，直接让分配器记账
template <class T>
struct CountingAllocator {
    using value_type = T;

    CountingAllocator() = default;
    template <class U>
    CountingAllocator(const CountingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        AllocationStats::bytes += n * sizeof(T);
        AllocationStats::blocks += 1;
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept {
        std::allocator<T>{}.deallocate(p, n);
    }

    template <class U>
    bool operator==(const CountingAllocator<U>&) const noexcept { return true; }
    template <class U>
    bool operator!=(const CountingAllocator<U>&) const noexcept { return false; }
};

using Data = std::pair<int, std::string>;

// 同一份比较逻辑服务排序和二分查找：键在前、键在后都要能比
struct ById {
    static inline long comparisons = 0;

    bool operator()(const Data& a, const Data& b) const {
        ++comparisons;
        return a.first < b.first;
    }
    bool operator()(const Data& a, int key) const {
        ++comparisons;
        return a.first < key;
    }
    bool operator()(int key, const Data& b) const {
        ++comparisons;
        return key < b.first;
    }
};

struct CountingLess {
    static inline long comparisons = 0;
    bool operator()(int a, int b) const {
        ++comparisons;
        return a < b;
    }
};

constexpr int kCount = 2000;

int main() {
    using Map = std::map<int, std::string, CountingLess,
                         CountingAllocator<std::pair<const int, std::string>>>;
    using Flat = std::vector<Data, CountingAllocator<Data>>;

    AllocationStats::reset();
    Map tree;
    for (int i = 0; i < kCount; ++i) {
        tree.emplace(i, "city" + std::to_string(i % 50));
    }
    const auto tree_bytes = AllocationStats::bytes;
    const auto tree_blocks = AllocationStats::blocks;

    AllocationStats::reset();
    Flat flat;
    flat.reserve(kCount);  // 一次性留够，下面的 emplace_back 不再分配
    for (int i = 0; i < kCount; ++i) {
        flat.emplace_back(i, "city" + std::to_string(i % 50));
    }
    std::sort(flat.begin(), flat.end(), ById{});
    const auto flat_bytes = AllocationStats::bytes;
    const auto flat_blocks = AllocationStats::blocks;

    std::cout << "元素个数: " << kCount << '\n';
    std::cout << "map 分配 " << tree_bytes << " 字节，分 " << tree_blocks << " 块\n";
    std::cout << "排序 vector 分配 " << flat_bytes << " 字节，分 " << flat_blocks << " 块\n";
    std::cout << "每个元素平均: map " << tree_bytes / kCount
              << " 字节，vector " << flat_bytes / kCount << " 字节\n";

    // 查询阶段：同样的键，一边走红黑树，一边走二分查找
    CountingLess::comparisons = 0;
    ById::comparisons = 0;
    std::size_t found_tree = 0;
    std::size_t found_flat = 0;
    for (int k = 0; k < kCount; k += 7) {
        if (auto it = tree.find(k); it != tree.end()) {
            found_tree += it->second.size();
        }
        auto it = std::lower_bound(flat.begin(), flat.end(), k, ById{});
        if (it != flat.end() && it->first == k) {
            found_flat += it->second.size();
        }
    }
    std::cout << "查找校验和: map " << found_tree << "，vector " << found_flat << '\n';
    std::cout << "比较次数: map " << CountingLess::comparisons
              << "，vector " << ById::comparisons << '\n';

    // 建立、查找、整体重建：重建时重新排序，而不是逐条插入
    for (auto& [id, city] : flat) {
        if (id % 2 == 0) {
            city = "rebuilt";
        }
    }
    std::sort(flat.begin(), flat.end(), ById{});
    std::cout << "重建后仍是 " << flat.size() << " 条，首尾键 "
              << flat.front().first << ".." << flat.back().first << '\n';
}
