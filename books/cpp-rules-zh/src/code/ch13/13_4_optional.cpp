// 第 13.4 节：optional 把“可能没有值”放进类型，0 不再兼任失败标记
#include <iostream>
#include <map>
#include <optional>
#include <string>

class PriceBook {
public:
    void set(std::string sku, int price) { prices_[std::move(sku)] = price; }

    std::optional<int> lookup(const std::string& sku) const {
        const auto it = prices_.find(sku);
        if (it == prices_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

private:
    std::map<std::string, int> prices_;
};

int main() {
    PriceBook book;
    book.set("coffee", 0);
    book.set("tea", 3);

    if (const auto price = book.lookup("coffee")) {
        std::cout << "coffee 的价格：" << *price << "（有值，值是 0）\n";
    }

    if (const auto price = book.lookup("juice")) {
        std::cout << "juice 的价格：" << *price << "\n";
    } else {
        std::cout << "juice：没有这个商品（和价格为 0 是两件事）\n";
    }

    const auto fallback = book.lookup("juice").value_or(99);
    std::cout << "juice 用默认价：" << fallback << "\n";
}
