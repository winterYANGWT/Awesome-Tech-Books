// 第 5.5 节：move 无条件转右值，forward 按 T 中编码的值类别转
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

struct Item {
    static int copies;
    static int moves;

    std::string name;
    bool moved_from = false;

    explicit Item(std::string n) : name(std::move(n)) {}
    Item(const Item& other) : name(other.name), moved_from(other.moved_from) {
        ++copies;
    }
    Item(Item&& other) noexcept
        : name(std::move(other.name)), moved_from(other.moved_from) {
        other.moved_from = true;
        ++moves;
    }
    Item& operator=(const Item&) = default;
    Item& operator=(Item&&) = default;
};

int Item::copies = 0;
int Item::moves = 0;

// 只读的下一层：拷贝
void sink(const Item& item) {
    std::cout << "  sink(const Item&)：" << item.name << "\n";
}

// 接受右值的下一层：真的把值搬进局部对象，源对象被标记
void sink(Item&& item) {
    Item taken = std::move(item);
    std::cout << "  sink(Item&&)：" << taken.name << "\n";
}

template <typename T>
void relay_good(T&& item) {
    sink(std::forward<T>(item));   // 条件转换
}

template <typename T>
void relay_bad(T&& item) {
    sink(std::move(item));         // 无条件转换，左值也会被搬
}

int main() {
    std::cout << "--- 左值走 forward ---\n";
    Item local("local");
    relay_good(local);
    std::cout << "local.moved_from = " << local.moved_from << "\n";

    std::cout << "--- 右值走 forward ---\n";
    relay_good(Item("temporary"));

    std::cout << "--- 左值走 move ---\n";
    Item victim("victim");
    relay_bad(victim);
    std::cout << "victim.moved_from = " << victim.moved_from << "\n";

    std::cout << "--- const 对象上的 move ---\n";
    const Item frozen("frozen");
    Item thawed = std::move(frozen);   // const Item&& 只能匹配拷贝构造
    std::cout << "frozen.moved_from = " << frozen.moved_from
              << "，thawed.name = " << thawed.name << "\n";

    // std::move 本身不改对象，只有真正调用移动构造/赋值才改
    Item untouched("untouched");
    auto&& rvalue_ref = std::move(untouched);
    static_assert(std::is_same_v<decltype(rvalue_ref), Item&&>);
    (void)rvalue_ref;
    std::cout << "只做 move 之后 untouched.moved_from = "
              << untouched.moved_from << "\n";

    std::cout << "拷贝 " << Item::copies << " 次，移动 " << Item::moves << " 次\n";
}
