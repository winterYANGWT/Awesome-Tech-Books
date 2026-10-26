// 第 23.1 节：用初始化捕获把只可移动或拷贝昂贵的对象移进闭包
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class Payload {
public:
    Payload(std::initializer_list<int> values) : values_(values) {}
    Payload(const Payload& other) : values_(other.values_) { ++copies_; }
    Payload(Payload&& other) noexcept : values_(std::move(other.values_)) { ++moves_; }

    std::size_t size() const { return values_.size(); }

    static int copies() { return copies_; }
    static int moves() { return moves_; }

private:
    std::vector<int> values_;
    static inline int copies_ = 0;
    static inline int moves_ = 0;
};

class Report {
public:
    explicit Report(int step) : step_(step) {}

    // 返回的闭包按值保存 step_ 的副本，不保存 Report 的 this
    auto stepFilter() const {
        return [step = step_](int value) { return value % step == 0; };
    }

private:
    int step_;
};

int main() {
    Payload payload{1, 2, 3, 4};
    auto owned = std::make_unique<Payload>(std::initializer_list<int>{5, 6});

    // 左边是闭包类里的成员名，右边是定义处可见的表达式
    auto sizes = [payload = std::move(payload), owned = std::move(owned)] {
        return payload.size() + owned->size();
    };
    static_assert(!std::is_copy_constructible_v<decltype(sizes)>,
                  "含 unique_ptr 的闭包只能移动，不能复制");

    std::printf("闭包里的元素总数 = %zu\n", sizes());
    std::printf("Payload 的拷贝 %d 次，移动 %d 次\n", Payload::copies(), Payload::moves());

    std::function<bool(int)> saved;
    {
        Report report(3);
        saved = report.stepFilter();  // 只复制 step_，report 可以立刻销毁
    }
    std::printf("Report 销毁后，9 能被 3 整除：%d\n", saved(9) ? 1 : 0);
}
