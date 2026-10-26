// 第 12.5 节：标准容器的保证层级，以及元素类型会不会抛的影响
// mode: run
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Bomb {
    std::string tag;
    static inline int copies_left = -1;
    static inline int assigns_left = -1;

    explicit Bomb(std::string text) : tag(std::move(text)) {}

    Bomb(const Bomb& other) : tag(other.tag) {
        if (copies_left >= 0 && copies_left-- == 0) {
            throw std::runtime_error("copy failed");
        }
    }

    Bomb& operator=(const Bomb& other) {
        if (assigns_left >= 0 && assigns_left-- == 0) {
            throw std::runtime_error("assignment failed");
        }
        tag = other.tag;
        return *this;
    }
};

// 只提供输入迭代器：实现只能一个一个地读，无法预先知道区间长度
struct InputIt {
    using iterator_category = std::input_iterator_tag;
    using value_type = Bomb;
    using difference_type = std::ptrdiff_t;
    using pointer = const Bomb*;
    using reference = const Bomb&;

    const std::vector<Bomb>* source = nullptr;
    std::size_t index = 0;

    reference operator*() const { return (*source)[index]; }
    InputIt& operator++() { ++index; return *this; }
    InputIt operator++(int) { InputIt old = *this; ++index; return old; }
    bool operator==(const InputIt& other) const { return index == other.index; }
    bool operator!=(const InputIt& other) const { return !(*this == other); }
};

template <class Container>
void dump(const char* label, const Container& container) {
    std::printf("%s size=%zu:", label, container.size());
    for (const Bomb& item : container) {
        std::printf(" %s", item.tag.c_str());
    }
    std::printf("\n");
}

void reset() {
    Bomb::copies_left = -1;
    Bomb::assigns_left = -1;
}

int main() {
    // 1. 单元素 push_back：重分配时复制失败，vector 保持原样（强保证）
    {
        std::vector<Bomb> items;
        items.reserve(3);
        items.emplace_back("a");
        items.emplace_back("b");
        items.emplace_back("c");
        Bomb::copies_left = 1;
        try {
            items.emplace_back("d");
        } catch (const std::exception& e) {
            std::printf("push_back failed: %s\n", e.what());
        }
        reset();
        dump("push_back", items);
    }

    // 2. 单元素 insert：元素赋值抛出时，vector 不做强保证
    {
        std::vector<Bomb> items;
        items.reserve(10);
        items.emplace_back("a");
        items.emplace_back("b");
        items.emplace_back("c");
        items.emplace_back("d");
        Bomb::assigns_left = 1;
        try {
            items.insert(items.begin() + 1, Bomb("X"));
        } catch (const std::exception& e) {
            std::printf("single insert failed: %s\n", e.what());
        }
        reset();
        dump("single insert", items);
    }

    // 3. vector 的区间插入：标准只给基本保证
    {
        std::vector<Bomb> items;
        items.reserve(10);
        items.emplace_back("a");
        items.emplace_back("b");

        std::vector<Bomb> add;
        add.emplace_back("X");
        add.emplace_back("Y");
        add.emplace_back("Z");

        Bomb::copies_left = 1;
        try {
            items.insert(items.end(), InputIt{&add, 0}, InputIt{&add, add.size()});
        } catch (const std::exception& e) {
            std::printf("vector range failed: %s\n", e.what());
        }
        reset();
        dump("vector range", items);
    }

    // 4. list 的区间插入：失败时容器不变（强保证）
    {
        std::list<Bomb> items;
        items.emplace_back("a");
        items.emplace_back("b");

        std::vector<Bomb> add;
        add.emplace_back("X");
        add.emplace_back("Y");
        add.emplace_back("Z");

        Bomb::copies_left = 1;
        try {
            items.insert(items.end(), InputIt{&add, 0}, InputIt{&add, add.size()});
        } catch (const std::exception& e) {
            std::printf("list range failed: %s\n", e.what());
        }
        reset();
        dump("list range", items);
    }
}
