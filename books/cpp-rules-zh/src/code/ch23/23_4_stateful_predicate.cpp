// 第 23.4 节：带状态的谓词会被算法拷贝，删掉的是副本数到的位置
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <vector>

// 不是纯函数：结果取决于 calls_，而 calls_ 每次调用都变
class FlagNth {
public:
    explicit FlagNth(std::size_t n) : n_(n) {}
    bool operator()(int) { return ++calls_ == n_; }
    std::size_t calls() const { return calls_; }

private:
    std::size_t calls_ = 0;
    std::size_t n_;
};

// 把 operator() 写成 const 也挡不住：mutable 成员照样能改
class FlagNthMutable {
public:
    explicit FlagNthMutable(std::size_t n) : n_(n) {}
    bool operator()(int) const { return ++calls_ == n_; }
    std::size_t calls() const { return calls_; }

private:
    mutable std::size_t calls_ = 0;
    std::size_t n_;
};

template <typename Predicate>
std::vector<int> removeMatching(std::vector<int> values, Predicate pred) {
    auto tail = std::remove_if(values.begin(), values.end(), pred);
    values.erase(tail, values.end());
    return values;
}

void print(const char* label, const std::vector<int>& values) {
    std::printf("%s", label);
    for (int value : values) {
        std::printf(" %d", value);
    }
    std::printf("\n");
}

int main() {
    const std::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8};

    FlagNth third(3);  // 想删“第三个元素”
    print("带计数的谓词：", removeMatching(numbers, third));
    std::printf("  但原来的谓词对象被调用 %zu 次\n", third.calls());

    // 非 const 的 operator() 不能用在 const 谓词对象上
    static_assert(!std::is_invocable_v<const FlagNth&, int>);

    FlagNthMutable thirdMutable(3);
    print("mutable 计数的谓词：", removeMatching(numbers, thirdMutable));
    std::printf("  原对象同样被调用 %zu 次\n", thirdMutable.calls());

    // 纯函数：只看实参，副本之间没有区别
    auto isOdd = [](int value) { return value % 2 != 0; };
    print("纯谓词（删奇数）：", removeMatching(numbers, isOdd));
}
