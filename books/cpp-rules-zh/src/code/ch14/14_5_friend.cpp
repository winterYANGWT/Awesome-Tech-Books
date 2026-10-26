// 第 14.5 节：只把 friend 授给真正需要访问私有状态的那个函数
#include <iostream>
#include <string>
#include <utility>

class Account {
public:
    Account(std::string owner, long cents) : owner_(std::move(owner)), cents_(cents) {}

    [[nodiscard]] const std::string& owner() const { return owner_; }
    [[nodiscard]] long cents() const { return cents_; }

    // 只有这个非成员函数需要同时检查并修改两个对象的私有余额。
    friend void transfer(Account& from, Account& to, long amount);

private:
    std::string owner_;
    long cents_;
};

void transfer(Account& from, Account& to, long amount) {
    if (amount <= 0 || from.cents_ < amount) {
        std::cout << "transfer rejected\n";
        return;
    }
    from.cents_ -= amount;
    to.cents_ += amount;
}

// 只读审计不是友元：公开的 owner()/cents() 已经够用。
void audit(const Account& a) {
    std::cout << a.owner() << "=" << a.cents() << "\n";
}

int main() {
    Account alice("alice", 500);
    Account bob("bob", 100);

    transfer(alice, bob, 200);
    audit(alice);
    audit(bob);
    std::cout << "total=" << alice.cents() + bob.cents() << "\n";

    transfer(alice, bob, 1000);
    std::cout << "total=" << alice.cents() + bob.cents() << "\n";
}
