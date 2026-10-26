// 第 6.2 节：按值传基类会切片；需要完整复制时用 Clone
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Account {
public:
    Account(std::string owner, long cents)
        : owner_(std::move(owner)), cents_(cents) {}
    virtual ~Account() = default;

    virtual const char* kind() const { return "Account"; }
    virtual std::unique_ptr<Account> clone() const {
        return std::make_unique<Account>(*this);
    }

    const std::string& owner() const { return owner_; }
    long cents() const { return cents_; }

private:
    std::string owner_;
    long cents_;
};

class Savings : public Account {
public:
    Savings(std::string owner, long cents, double rate)
        : Account(std::move(owner), cents), rate_(rate) {}

    const char* kind() const override { return "Savings"; }
    double rate() const { return rate_; }

    std::unique_ptr<Account> clone() const override {
        return std::make_unique<Savings>(*this);
    }

private:
    double rate_ = 0.0;
};

// 形参是基类对象：实参的派生部分被切掉，虚函数也按基类对象调用。
void audit_by_value(Account account) {
    std::cout << "按值: kind=" << account.kind()
              << " owner=" << account.owner() << '\n';
}

void audit_by_reference(const Account& account) {
    std::cout << "按引用: kind=" << account.kind()
              << " owner=" << account.owner() << '\n';
}

int main() {
    Savings savings("Ada", 125000, 0.032);

    audit_by_value(savings);       // 切片：kind 变成 Account
    audit_by_reference(savings);   // 保持动态类型

    std::unique_ptr<Account> copy = savings.clone();
    std::cout << "clone: kind=" << copy->kind()
              << " owner=" << copy->owner() << '\n';
    auto* as_savings = dynamic_cast<Savings*>(copy.get());
    std::cout << "clone 的利率: " << (as_savings ? as_savings->rate() : 0.0) << '\n';
}
