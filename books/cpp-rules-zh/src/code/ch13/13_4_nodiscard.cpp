// 第 13.4 节：[[nodiscard]] 能把“忽略 bool 返回值”变成编译期警告
// mode: warn
// expect: -Wunused-result
#include <map>
#include <string>

class PriceBook {
public:
    void set(std::string sku, int price) { prices_[std::move(sku)] = price; }

    [[nodiscard]] bool lookup(const std::string& sku, int& out) const {
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
    book.set("tea", 3);

    int price = -1;
    book.lookup("juice", price);  // 返回值被丢掉：clang 和 GCC 都会警告
}
