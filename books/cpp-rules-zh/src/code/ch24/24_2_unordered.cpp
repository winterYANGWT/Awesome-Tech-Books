// 第 24.2 节：非标准 hash_* 已成历史，C++11 起用 unordered_*
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

struct PartNumber {
    std::string prefix;
    int number;

    bool operator==(const PartNumber& other) const {
        return prefix == other.prefix && number == other.number;
    }
};

struct PartHash {
    static inline long calls = 0;

    std::size_t operator()(const PartNumber& part) const {
        ++calls;
        // 一个够用的组合：字符串哈希左移后和整数哈希异或
        return std::hash<std::string>{}(part.prefix)
             ^ (std::hash<int>{}(part.number) << 1);
    }
};

struct PartEqual {
    static inline long calls = 0;

    bool operator()(const PartNumber& a, const PartNumber& b) const {
        ++calls;
        return a == b;
    }
};

int main() {
    std::cout << std::boolalpha;
    using Index = std::unordered_map<PartNumber, int, PartHash, PartEqual>;

    Index stock;
    stock.reserve(1000);  // 先留够桶，插入时就不必反复 rehash
    for (int i = 0; i < 1000; ++i) {
        stock.emplace(PartNumber{"BOLT", i}, i * 2);
    }

    std::cout << "桶数: " << stock.bucket_count() << '\n';
    std::cout << "负载因子: " << stock.load_factor() << '\n';
    std::cout << "最大负载因子: " << stock.max_load_factor() << '\n';
    std::cout << "元素个数: " << stock.size() << '\n';

    PartHash::calls = 0;
    PartEqual::calls = 0;
    int sum = 0;
    for (int i = 0; i < 1000; i += 3) {
        if (auto it = stock.find(PartNumber{"BOLT", i}); it != stock.end()) {
            sum += it->second;
        }
    }
    const long lookups = 334;
    std::cout << "命中校验和: " << sum << '\n';
    std::cout << "平均每次查找: 哈希 " << static_cast<double>(PartHash::calls) / lookups
              << " 次，相等比较 " << static_cast<double>(PartEqual::calls) / lookups
              << " 次\n";

    // C++20 起可以直接问 contains，不必拿 find 和 end() 比
    std::cout << "contains(BOLT, 500): " << stock.contains(PartNumber{"BOLT", 500}) << '\n';
    std::cout << "contains(BOLT, 5000): " << stock.contains(PartNumber{"BOLT", 5000}) << '\n';

    // 无序容器的遍历顺序不按键排序，需要有序输出时要自己排
    std::cout << "遍历顺序是否按键升序: ";
    bool sorted = true;
    int previous = -1;
    for (const auto& [part, count] : stock) {
        (void)count;
        if (part.number < previous) {
            sorted = false;
        }
        previous = part.number;
    }
    std::cout << sorted << '\n';
}
