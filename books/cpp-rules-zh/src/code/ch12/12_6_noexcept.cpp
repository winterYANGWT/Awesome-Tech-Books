// 第 12.6 节：noexcept 进入函数类型，并决定 vector 扩容时移动还是复制
// mode: run
#include <cstdio>
#include <type_traits>
#include <utility>
#include <vector>

struct SafeToken {
    static inline int moves = 0;
    static inline int copies = 0;
    int id;

    explicit SafeToken(int value) : id(value) {}
    SafeToken(const SafeToken& other) : id(other.id) { ++copies; }
    SafeToken(SafeToken&& other) noexcept : id(other.id) { ++moves; }
};

struct RiskyToken {
    static inline int moves = 0;
    static inline int copies = 0;
    int id;

    explicit RiskyToken(int value) : id(value) {}
    RiskyToken(const RiskyToken& other) : id(other.id) { ++copies; }
    RiskyToken(RiskyToken&& other) : id(other.id) { ++moves; }
};

void plain();
void noex() noexcept;

// C++17 起，noexcept 是函数类型的一部分
static_assert(!std::is_same_v<decltype(plain), decltype(noex)>);
static_assert(std::is_same_v<decltype(noex), void() noexcept>);

static_assert(std::is_nothrow_move_constructible_v<SafeToken>);
static_assert(!std::is_nothrow_move_constructible_v<RiskyToken>);

// move_if_noexcept：移动不抛才按右值搬，否则退回 const 左值复制
static_assert(std::is_same_v<decltype(std::move_if_noexcept(std::declval<SafeToken&>())),
                             SafeToken&&>);
static_assert(std::is_same_v<decltype(std::move_if_noexcept(std::declval<RiskyToken&>())),
                             const RiskyToken&>);

template <class T>
void grow(const char* label) {
    std::vector<T> items;
    items.reserve(1);
    items.emplace_back(0);
    for (int i = 1; i < 20; ++i) {
        items.emplace_back(i);
    }
    std::printf("%s: moves=%d copies=%d\n", label, T::moves, T::copies);
}

int main() {
    grow<SafeToken>("noexcept move");
    grow<RiskyToken>("throwing move");

    static_assert(std::is_nothrow_swappable_v<std::vector<int>>);
    std::printf("vector<int> swap is noexcept: %d\n",
                std::is_nothrow_swappable_v<std::vector<int>> ? 1 : 0);
}
