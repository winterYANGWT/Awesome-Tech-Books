// 第 23.3 节：函数对象按值传递，要廉价可拷贝；重状态藏进 Pimpl
#include <array>
#include <cstdio>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// 实现类：状态大（1024 字节的折扣表），还带虚函数
class DiscountImpl {
public:
    explicit DiscountImpl(int rate) : rate_(rate) {
        for (int i = 0; i < static_cast<int>(table_.size()); ++i) {
            table_[i] = i * (100 - rate_) / 100;
        }
    }
    virtual ~DiscountImpl() = default;
    virtual int apply(int price) const { return table_[price % table_.size()]; }

private:
    std::array<int, 256> table_{};
    int rate_;
};

// 外层：只有一个 shared_ptr，按值拷贝就是一次引用计数加一
class Discount {
public:
    explicit Discount(int rate) : impl_(std::make_shared<DiscountImpl>(rate)) {}
    Discount(const Discount& other) : impl_(other.impl_) { ++copies_; }

    int operator()(int price) const { return impl_->apply(price); }

    static int copies() { return copies_; }

private:
    std::shared_ptr<const DiscountImpl> impl_;
    static inline int copies_ = 0;
};

// 一个和 for_each 一样按值接收函数对象的算法，拷贝次数因此是确定的
template <typename Iter, typename Function>
void applyEach(Iter first, Iter last, Function f) {
    for (; first != last; ++first) {
        f(*first);
    }
}

int main() {
    std::printf("外层 Discount 大小 = %zu 字节，实现类大小 = %zu 字节\n",
                sizeof(Discount), sizeof(DiscountImpl));

    std::vector<int> prices{100, 150, 200, 250};
    Discount discount(25);

    applyEach(prices.begin(), prices.end(), discount);
    std::printf("按值传入：Discount 被拷贝 %d 次\n", Discount::copies());

    // std::ref 让算法持有引用包装器，不再拷贝 Discount 本身
    applyEach(prices.begin(), prices.end(), std::ref(discount));
    std::printf("std::ref 传入：Discount 仍被拷贝 %d 次\n", Discount::copies());

    // 关联容器的比较器必须是对象；lambda 的闭包类型正好是对象
    auto byLength = [](const std::string& a, const std::string& b) {
        return a.size() < b.size();
    };
    std::set<std::string, decltype(byLength)> names(byLength);
    names.insert({"ops", "billing", "eu-west"});
    std::printf("按长度排序后的首元素：%s\n", names.begin()->c_str());
}
