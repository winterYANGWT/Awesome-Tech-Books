// 第 29.1 节：把隐含的二次操作数出来：find 去重与哈希去重
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <vector>

struct OrderId {
    int value;

    // 计数放在类型里，输出是确定的：线性查找每次比较都会加一。
    static inline std::size_t comparisons = 0;
    static inline std::size_t hashes = 0;

    friend bool operator==(const OrderId& a, const OrderId& b) {
        ++comparisons;
        return a.value == b.value;
    }
};

template <>
struct std::hash<OrderId> {
    std::size_t operator()(const OrderId& id) const noexcept {
        ++OrderId::hashes;
        return std::hash<int>{}(id.value);
    }
};

// 对每个元素在已见结果里做一次线性 find：n 个元素，O(n^2) 次比较。
std::vector<OrderId> unique_linear(const std::vector<OrderId>& input) {
    std::vector<OrderId> result;
    for (const OrderId& id : input) {
        if (std::find(result.begin(), result.end(), id) == result.end()) {
            result.push_back(id);
        }
    }
    return result;
}

// 哈希集合的期望复杂度是 O(1) 查找，整体期望 O(n)。
std::vector<OrderId> unique_hashed(const std::vector<OrderId>& input) {
    std::unordered_set<OrderId> seen;
    seen.reserve(input.size() * 2);
    std::vector<OrderId> result;
    for (const OrderId& id : input) {
        if (seen.insert(id).second) {
            result.push_back(id);
        }
    }
    return result;
}

int main() {
    for (const int n : {500, 1000, 2000}) {
        std::vector<OrderId> input;
        input.reserve(static_cast<std::size_t>(n));
        // 故意全部不重复：这是线性查找的最坏情况。
        for (int i = 0; i < n; ++i) input.push_back(OrderId{i});

        OrderId::comparisons = 0;
        OrderId::hashes = 0;
        const auto linear = unique_linear(input);
        const std::size_t linear_cmp = OrderId::comparisons;

        OrderId::comparisons = 0;
        OrderId::hashes = 0;
        const auto hashed = unique_hashed(input);
        const std::size_t hashed_cmp = OrderId::comparisons;
        const std::size_t hashed_hash = OrderId::hashes;

        std::cout << "n=" << n
                  << "  linear: " << linear.size() << " 个结果, "
                  << linear_cmp << " 次比较"
                  << "  hashed: " << hashed.size() << " 个结果, "
                  << hashed_hash << " 次哈希, " << hashed_cmp << " 次比较\n";
    }
}
