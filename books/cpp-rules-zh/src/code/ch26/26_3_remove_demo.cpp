// 第 26.3 节：remove 只把保留元素前移并返回新逻辑尾，erase 才改变大小
#include <algorithm>
#include <cstdio>
#include <list>
#include <set>
#include <vector>

void print(const char* label, const std::vector<int>& v) {
    std::printf("%s:", label);
    if (v.empty()) std::printf(" <空>");
    for (int x : v) std::printf(" %d", x);
    std::puts("");
}

int main() {
    std::vector<int> cart{101, 102, 101, 103, 101, 104};
    std::printf("删除前 size=%zu\n", cart.size());

    auto logicalEnd = std::remove(cart.begin(), cart.end(), 101);
    std::printf("remove 后 size=%zu，逻辑尾之前有 %td 个元素\n",
                cart.size(), logicalEnd - cart.begin());
    std::printf("逻辑区间内:");
    for (auto it = cart.begin(); it != logicalEnd; ++it) std::printf(" %d", *it);
    std::puts("");

    // 逻辑尾之后的值是未指定的，不能依赖；真正删除要交给容器的 erase
    cart.erase(logicalEnd, cart.end());
    std::printf("erase 后 size=%zu\n", cart.size());
    print("剩余", cart);

    // C++20：一步完成按值删除和按谓词删除，返回删除个数
    auto removed = std::erase(cart, 103);
    std::printf("std::erase 删除 %zu 个 103\n", removed);
    auto evicted = std::erase_if(cart, [](int id) { return id % 2 == 0; });
    std::printf("std::erase_if 删除 %zu 个偶数\n", evicted);
    print("最终", cart);

    // list 的成员 remove 直接改链，C++20 起还返回删除个数
    std::list<int> playlist{7, 8, 8, 9, 8};
    auto dropped = playlist.remove(8);
    std::printf("list::remove 删除 %zu 个，size=%zu\n", dropped, playlist.size());

    // 关联容器按值删除用成员 erase，按等价比较，对数时间
    std::set<int> badges{5, 6, 7};
    auto erased = badges.erase(6);
    std::printf("set::erase(6) 返回 %zu，size=%zu\n", erased, badges.size());
}
