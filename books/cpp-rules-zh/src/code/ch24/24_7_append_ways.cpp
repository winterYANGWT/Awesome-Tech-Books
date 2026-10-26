// 第 24.7 节：往尾部加元素，push_back/resize/insert 的构造与赋值次数不同
#include <iostream>
#include <vector>

struct Item {
    int id = 0;
    static inline int defaults = 0;
    static inline int values = 0;
    static inline int copies = 0;
    static inline int moves = 0;
    static inline int copy_assigns = 0;
    static inline int move_assigns = 0;

    Item() { ++defaults; }
    explicit Item(int i) : id(i) { ++values; }
    Item(const Item& other) : id(other.id) { ++copies; }
    Item(Item&& other) noexcept : id(other.id) { ++moves; }
    Item& operator=(const Item& other) {
        id = other.id;
        ++copy_assigns;
        return *this;
    }
    Item& operator=(Item&& other) noexcept {
        id = other.id;
        ++move_assigns;
        return *this;
    }
};

void reset() {
    Item::defaults = Item::values = Item::copies = Item::moves = 0;
    Item::copy_assigns = Item::move_assigns = 0;
}

void report(const char* how) {
    std::cout << how << ": 默认构造 " << Item::defaults
              << "，值构造 " << Item::values
              << "，拷贝构造 " << Item::copies
              << "，移动构造 " << Item::moves
              << "，拷贝赋值 " << Item::copy_assigns
              << "，移动赋值 " << Item::move_assigns << '\n';
}

int main() {
    constexpr int kCount = 6;

    reset();
    {
        std::vector<Item> v;
        v.reserve(kCount);
        for (int i = 0; i < kCount; ++i) {
            v.push_back(Item(i));
        }
    }
    report("push_back(Item(i))");

    reset();
    {
        std::vector<Item> v;
        v.reserve(kCount);
        for (int i = 0; i < kCount; ++i) {
            v.resize(v.size() + 1);  // 先默认构造一个新槽位
            v.back() = Item(i);      // 再赋值进去
        }
    }
    report("resize 后赋值");

    reset();
    {
        std::vector<Item> v;
        v.reserve(kCount);
        for (int i = 0; i < kCount; ++i) {
            v.insert(v.end(), Item(i));  // 语义上是“在末尾插入”
        }
    }
    report("insert(end(), Item(i))");

    reset();
    {
        std::vector<Item> v;
        v.reserve(kCount);
        for (int i = 0; i < kCount; ++i) {
            v.emplace_back(i);  // 实参直接传给 Item(int)
        }
    }
    report("emplace_back(i)");
}
