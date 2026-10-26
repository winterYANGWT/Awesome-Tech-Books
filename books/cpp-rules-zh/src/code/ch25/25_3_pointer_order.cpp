// 第 25.2 节：set<string*> 默认按地址排序；解引用比较器才按内容
// mode: run
#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <type_traits>

// 通用解引用比较器：指针、迭代器、智能指针都适用
struct DereferenceLess {
    template <class Ptr>
    bool operator()(const Ptr& a, const Ptr& b) const {
        return *a < *b;
    }
};

// 默认比较器确实是 less<string*>，它比较的是地址
static_assert(std::is_same_v<std::set<std::string*>::key_compare, std::less<std::string*>>);

int main() {
    // 同一个数组的元素地址随下标递增，输出才是确定的
    std::array<std::string, 4> words{"Wombat", "Anteater", "Penguin", "Lemur"};

    std::set<std::string*> by_address;
    for (auto& word : words) {
        by_address.insert(&word);
    }
    std::cout << "默认 set<string*>（地址升序，也就是数组下标顺序）：\n";
    for (auto* p : by_address) {
        std::cout << "  " << *p << '\n';
    }

    std::set<std::string*, DereferenceLess> by_text;
    for (auto& word : words) {
        by_text.insert(&word);
    }
    std::cout << "DereferenceLess 之后（字符串内容升序）：\n";
    for (auto* p : by_text) {
        std::cout << "  " << *p << '\n';
    }

    // 智能指针默认也按地址排序，同一个解引用比较器同样能用
    std::set<std::shared_ptr<std::string>, DereferenceLess> shared;
    shared.insert(std::make_shared<std::string>("Zebra"));
    shared.insert(std::make_shared<std::string>("Aardvark"));
    std::cout << "shared_ptr 容器按内容排序：";
    for (const auto& p : shared) {
        std::cout << ' ' << *p;
    }
    std::cout << '\n';
}
