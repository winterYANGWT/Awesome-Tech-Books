// 第 2.6 节：const 是接口契约，mutable 只用于不影响可观察状态的缓存
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class PriceBook {
public:
    void add(std::string item, int cents) {
        items_.push_back({std::move(item), cents});
        dirty_ = true;
    }

    // 逻辑 const：total() 不改可观察状态，缓存用 mutable。
    int total() const {
        if (dirty_) {
            cached_ = compute();
            dirty_ = false;
        }
        return cached_;
    }

    const std::string& name() const { return name_; }
    std::string& name() { return name_; }

private:
    struct Item {
        std::string name;
        int cents;
    };

    int compute() const {
        int sum = 0;
        for (const auto& item : items_) sum += item.cents;
        return sum;
    }

    std::vector<Item> items_;
    std::string name_ = "price list";
    mutable int cached_ = 0;
    mutable bool dirty_ = true;
};

int main() {
    PriceBook book;
    book.add("apple", 120);
    book.add("bread", 300);

    const PriceBook& cbook = book;
    std::cout << cbook.name() << ": " << cbook.total() << " cents\n";
    std::cout << "again: " << cbook.total() << " cents\n";

    // const 对象只能拿到 const 引用；非 const 对象拿到可修改的引用。
    static_assert(std::is_same_v<decltype(cbook.name()), const std::string&>);
    static_assert(std::is_same_v<decltype(book.name()), std::string&>);
}
