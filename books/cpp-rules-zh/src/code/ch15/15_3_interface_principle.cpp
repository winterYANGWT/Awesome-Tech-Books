// 第 15.3 节：与类同处的自由函数属于类接口，ADL 会把两份接口摆到一起
// mode: fail
// expect: ambiguous
#include <iostream>
#include <ostream>

namespace billing {

class Invoice {
public:
    explicit Invoice(int cents) : cents_(cents) {}
    int cents() const { return cents_; }

private:
    int cents_;
};

// 与 Invoice 同处 billing 命名空间：按接口原则，它是 Invoice 接口的一部分。
std::ostream& operator<<(std::ostream& os, const Invoice& inv) {
    return os << "Invoice(" << inv.cents() << " cents)";
}

}  // namespace billing

namespace audit {

// 审计模块也想打印 Invoice，于是又写了一个“同处”的 operator<<。
std::ostream& operator<<(std::ostream& os, const billing::Invoice& inv) {
    return os << "audit: " << inv.cents();
}

void dump(const billing::Invoice& inv) {
    std::cout << inv << '\n';  // 二义：billing 的和 audit 的两个版本都可见
}

}  // namespace audit

int main() { audit::dump(billing::Invoice{4200}); }
