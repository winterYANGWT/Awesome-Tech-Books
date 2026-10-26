// 第 7.1 节：容器和元素的移动不一定便宜，vector 扩容还可能改选拷贝
#include <array>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Item {
    static int copies;
    static int moves;
    std::string name;

    explicit Item(std::string n) : name(std::move(n)) {}
    Item(const Item& other) : name(other.name) { ++copies; }
    Item(Item&& other) noexcept : name(std::move(other.name)) { ++moves; }
    Item& operator=(const Item&) = default;
    Item& operator=(Item&&) noexcept = default;
};
int Item::copies = 0;
int Item::moves = 0;

// 移动构造函数没有 noexcept：vector 扩容为强异常安全会改选拷贝。
struct SlowItem {
    static int copies;
    static int moves;
    int value;

    explicit SlowItem(int v) : value(v) {}
    SlowItem(const SlowItem& other) : value(other.value) { ++copies; }
    SlowItem(SlowItem&& other) noexcept(false) : value(other.value) { ++moves; }
};
int SlowItem::copies = 0;
int SlowItem::moves = 0;

void reset() {
    Item::copies = Item::moves = 0;
    SlowItem::copies = SlowItem::moves = 0;
}

int main() {
    // std::array 把元素直接放在对象里，移动只能逐个元素搬。
    std::array<Item, 4> tray{Item("bolt"), Item("nut"), Item("washer"), Item("screw")};
    reset();
    std::array<Item, 4> packed = std::move(tray);
    std::cout << "std::array 移动：拷贝 " << Item::copies
              << " 次，移动 " << Item::moves << " 次\n";

    // vector 把元素放在堆上，移动只要偷走三个指针。
    std::vector<Item> shelf;
    shelf.reserve(4);
    shelf.emplace_back("bolt");
    shelf.emplace_back("nut");
    shelf.emplace_back("washer");
    shelf.emplace_back("screw");
    reset();
    std::vector<Item> moved_shelf = std::move(shelf);
    std::cout << "std::vector 移动：拷贝 " << Item::copies
              << " 次，移动 " << Item::moves << " 次\n";

    // 左值不会自动变成移动源；不写 std::move，调用的就是拷贝。
    Item original("hinge");
    reset();
    Item copied = original;
    Item stolen = std::move(original);
    std::cout << "左值初始化拷贝 " << Item::copies
              << " 次，std::move 后移动 " << Item::moves << " 次\n";

    // 扩容时旧元素要搬到新缓冲区：移动标了 noexcept，就放心移动。
    std::vector<Item> fast;
    fast.reserve(1);
    fast.emplace_back("first");
    reset();
    fast.emplace_back("second");
    std::cout << "扩容时 noexcept 移动：拷贝 " << Item::copies
              << " 次，移动 " << Item::moves << " 次\n";

    // 同样扩容，但移动可能抛，vector 为强异常安全退回拷贝。
    std::vector<SlowItem> parts;
    parts.reserve(1);
    parts.emplace_back(1);
    reset();
    parts.emplace_back(2);
    std::cout << "扩容时移动未标 noexcept：拷贝 " << SlowItem::copies
              << " 次，移动 " << SlowItem::moves << " 次\n";

    std::cout << "两批货物：" << packed.size() << " + " << moved_shelf.size()
              << " 件，copied = " << copied.name << "\n";
}
