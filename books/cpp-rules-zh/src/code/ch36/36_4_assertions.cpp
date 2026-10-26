// 第 36.4 节：assert 记录内部不变式，static_assert 把编译期条件写进类型检查
// mode: run
#include <cassert>
#include <iostream>

class Account {
public:
    explicit Account(int balance) : balance_(balance) {
        assert(balance_ >= 0 && "initial balance must not be negative");
    }

    void withdraw(int amount) {
        assert(amount > 0 && "withdrawal amount must be positive");
        assert(amount <= balance_ && "withdrawal must not exceed the balance");
        balance_ -= amount;
        assert(balance_ >= 0 && "balance invariant");
    }

    int balance() const {
        assert(balance_ >= 0);
        return balance_;
    }

private:
    int balance_;
};

// 类型层面的不变式：账目余额用 int，Account 至少装得下一个 int
static_assert(sizeof(Account) >= sizeof(int));

int main() {
    Account account(100);
    account.withdraw(30);
    account.withdraw(20);
    std::cout << "balance after two withdrawals: " << account.balance() << '\n';
}
