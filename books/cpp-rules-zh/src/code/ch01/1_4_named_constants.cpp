// 第 1.4 节：把散落的数字收成具名常量，作用域跟着用途走

#include <array>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <numbers>
#include <string_view>

namespace shop {

constexpr int kUnitPriceCents = 1299;
constexpr int kTaxPercent = 19;
constexpr int kFreeShippingCents = 5000;
constexpr int kShippingCents = 600;
constexpr int kBulkUnits = 5;
constexpr int kBulkDiscountPercent = 10;
constexpr std::string_view kCurrency = "CNY";

}  // namespace shop

class Order {
public:
    static constexpr int kMaxLines = 8;

    bool add(int quantity) {
        if (line_count_ == kMaxLines || quantity <= 0) {
            return false;
        }
        quantities_[line_count_++] = quantity;
        return true;
    }

    int units() const {
        return std::accumulate(quantities_.begin(), quantities_.begin() + line_count_, 0);
    }

    int subtotal_cents() const { return units() * shop::kUnitPriceCents; }

private:
    std::array<int, kMaxLines> quantities_{};
    int line_count_ = 0;
};

int main() {
    Order order;
    order.add(2);
    order.add(4);

    const int subtotal = order.subtotal_cents();
    const int discount = order.units() >= shop::kBulkUnits
                             ? subtotal * shop::kBulkDiscountPercent / 100
                             : 0;
    const int after_discount = subtotal - discount;
    const int taxed = after_discount * (100 + shop::kTaxPercent) / 100;
    const int shipping = taxed >= shop::kFreeShippingCents ? 0 : shop::kShippingCents;
    const int total = taxed + shipping;

    std::cout << "units=" << order.units() << " subtotal=" << subtotal
              << " discount=" << discount << " tax=" << (taxed - after_discount)
              << " shipping=" << shipping << '\n';
    std::cout << "total=" << total << ' ' << shop::kCurrency << '\n';

    constexpr double radius = 2.0;
    const double area = std::numbers::pi * radius * radius;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "circle area=" << area << '\n';
}
