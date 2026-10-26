// 第 13.4 节：bool 加输出参数把“没有值”和“值为 0”混在一起
#include <iostream>
#include <map>
#include <string>

class PriceBook {
public:
    void set(std::string sku, int price) { prices_[std::move(sku)] = price; }

    // 旧接口：成功返回 true；失败返回 false，并把 out 写成 0。
    bool lookup(const std::string& sku, int& out) const {
        const auto it = prices_.find(sku);
        if (it == prices_.end()) {
            out = 0;
            return false;
        }
        out = it->second;
        return true;
    }

private:
    std::map<std::string, int> prices_;
};

int main() {
    PriceBook book;
    book.set("coffee", 0);  // 0 是合法价格：赠品
    book.set("tea", 3);

    int price = -1;
    book.lookup("coffee", price);
    std::cout << "coffee 的价格：" << price << "（0 元，合法）\n";

    int missing = -1;
    const bool found = book.lookup("juice", missing);
    std::cout << "juice 查找结果：found=" << std::boolalpha << found
              << " price=" << missing << "\n";

    // 调用方丢掉返回值，编译器不会提醒；missing 现在是 0，和赠品无法区分。
    book.lookup("juice", missing);
    std::cout << "忽略返回值后，juice 的价格显示为：" << missing
              << "，和 coffee 的 0 元看起来一样\n";
}
