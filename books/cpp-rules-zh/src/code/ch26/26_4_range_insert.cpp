// 第 26.4 节：区间插入让容器一次搬移；算法的目标区间不会自动扩容
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <vector>

struct Tracked {
    explicit Tracked(int v) : value(v) {}
    Tracked(const Tracked& other) : value(other.value) { ++copies; }
    Tracked(Tracked&& other) noexcept : value(other.value) { ++moves; }
    Tracked& operator=(const Tracked& other) {
        value = other.value;
        ++copies;
        return *this;
    }
    Tracked& operator=(Tracked&& other) noexcept {
        value = other.value;
        ++moves;
        return *this;
    }
    static void reset() { copies = moves = 0; }

    int value;
    static inline long copies = 0;
    static inline long moves = 0;
};

std::vector<Tracked> makeRange(int count, int first) {
    std::vector<Tracked> v;
    v.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) v.emplace_back(first + i);
    return v;
}

int main() {
    const auto source = makeRange(50, 1000);  // 源区间不参与计数

    std::vector<Tracked> oneByOne = makeRange(200, 0);
    oneByOne.reserve(300);  // 排除扩容的干扰，只看插入造成的搬移
    Tracked::reset();
    for (const Tracked& t : source) oneByOne.insert(oneByOne.begin() + 100, t);
    std::printf("逐个 insert：移动 %ld 次，拷贝 %ld 次\n", Tracked::moves, Tracked::copies);

    std::vector<Tracked> range = makeRange(200, 0);
    range.reserve(300);
    Tracked::reset();
    range.insert(range.begin() + 100, source.begin(), source.end());
    std::printf("区间 insert：移动 %ld 次，拷贝 %ld 次\n", Tracked::moves, Tracked::copies);
    std::printf("区间插入后 size=%zu\n", range.size());

    // 算法的目标区间不会自己变大；要追加就显式给插入迭代器
    std::vector<int> codes{3, 1, 4, 1, 5};
    std::vector<int> squares;
    squares.reserve(codes.size());
    std::transform(codes.begin(), codes.end(), std::back_inserter(squares),
                   [](int x) { return x * x; });
    std::printf("back_inserter：size=%zu，capacity=%zu\n",
                squares.size(), squares.capacity());
    std::printf("结果:");
    for (int x : squares) std::printf(" %d", x);
    std::puts("");
}
