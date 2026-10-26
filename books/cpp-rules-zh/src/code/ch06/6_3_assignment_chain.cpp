// 第 6.3 节：赋值返回 T&，链式赋值与容器约束才成立
#include <concepts>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

class Account {
public:
    Account(std::string owner, int balance)
        : owner_(std::move(owner)), balance_(balance) {}

    Account& operator=(const Account& rhs) {
        owner_ = rhs.owner_;
        balance_ = rhs.balance_;
        return *this;
    }

    Account& operator+=(int amount) {
        balance_ += amount;
        return *this;
    }

    const std::string& owner() const { return owner_; }
    int balance() const { return balance_; }

private:
    std::string owner_;
    int balance_ = 0;
};

// 反例：返回 const T&。表达式本身合法，但不满足 C++20 的 assignable_from。
class FrozenAccount {
public:
    FrozenAccount() = default;
    const FrozenAccount& operator=(const FrozenAccount&) { return *this; }
};

static_assert(std::is_same_v<
              decltype(std::declval<Account&>() = std::declval<const Account&>()),
              Account&>);
static_assert(std::assignable_from<Account&, const Account&>);
static_assert(!std::assignable_from<FrozenAccount&, const FrozenAccount&>);

// operator& 被重载时，&other 未必是对象的真实地址。
class OddAddress {
public:
    OddAddress* operator&() { return nullptr; }
    const OddAddress* operator&() const { return nullptr; }

    OddAddress& operator=(const OddAddress& other) {
        if (this != &other) {          // 骗得过这一行：&other 是 nullptr
            ++revision_;
        }
        return *this;
    }

    OddAddress& assign_with_addressof(const OddAddress& other) {
        if (this != std::addressof(other)) {   // 拿到真实地址，自赋值不增加
            ++revision_;
        }
        return *this;
    }

    int revision() const { return revision_; }

private:
    int revision_ = 0;
};

int main() {
    Account a("a", 1), b("b", 2), c("c", 3);
    a = b = c;                       // 右结合：先 c 赋给 b，再 b 赋给 a
    std::cout << "链式赋值: a=" << a.owner() << '/' << a.balance()
              << " b=" << b.owner() << '/' << b.balance() << '\n';

    (a += 10) += 5;                  // 复合赋值也返回引用
    std::cout << "复合赋值: a=" << a.balance() << '\n';

    OddAddress odd;
    OddAddress* alias = std::addressof(odd);   // 绕过字面自赋值警告
    odd = *alias;                    // this != &odd 为真
    std::cout << "重载 operator&: 自赋值后 revision=" << odd.revision() << '\n';

    OddAddress odd2;
    OddAddress* alias2 = std::addressof(odd2);
    odd2.assign_with_addressof(*alias2);
    std::cout << "std::addressof: 自赋值后 revision=" << odd2.revision() << '\n';
}
