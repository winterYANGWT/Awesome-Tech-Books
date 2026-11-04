// mode: run
#include <atomic>
#include <cstddef>
#include <iostream>

struct Item {
    int id;
    int payload;
};

int main() {
    Item items[4] = {{0, 10}, {1, 20}, {2, 30}, {3, 40}};
    std::atomic<Item*> p{&items[0]};

    std::cout << "sizeof(Item) = " << sizeof(Item) << '\n';

    Item* old = p.fetch_add(1);  // 按 sizeof(Item) 偏移，返回旧地址
    std::cout << "fetch_add: 旧地址 - 新地址 = " << (old - p.load())
              << " 个元素，字节差 = "
              << (reinterpret_cast<char*>(old) - reinterpret_cast<char*>(p.load()))
              << '\n';

    Item* post = p++;  // 后缀 ++ 返回旧地址
    std::cout << "p++ 返回 items[" << (post - items) << "]，p 现在指向 items["
              << (p.load() - items) << "]\n";

    Item* pre = ++p;  // 前缀 ++ 返回新地址
    std::cout << "++p 返回 items[" << (pre - items) << "]\n";

    Item* after_plus = (p += 1);  // += 返回新地址
    std::cout << "p += 1 返回 items[" << (after_plus - items) << "]\n";
}
