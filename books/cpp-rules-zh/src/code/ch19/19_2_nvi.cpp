// 第 19.2 节：NVI 用公开非虚函数包住私有虚函数，把前后逻辑固定在基类
#include <iostream>
#include <string>

class Account {
public:
    virtual ~Account() = default;

    // 公开、非虚：派生类不能改变“先检查金额、再记账”的顺序。
    std::string withdraw(int amount) {
        if (amount <= 0) {
            return "拒绝: 金额必须为正";
        }
        std::string trace = "检查金额; ";
        trace += do_withdraw(amount);
        trace += "; 写入流水";
        return trace;
    }

    int balance() const { return balance_; }

protected:
    explicit Account(int balance) : balance_(balance) {}

    bool take(int amount) {
        if (amount > balance_) {
            return false;
        }
        balance_ -= amount;
        return true;
    }

private:
    // 私有虚函数：派生类可以覆盖，但不能直接调用它。
    virtual std::string do_withdraw(int amount) = 0;

    int balance_;
};

class Checking : public Account {
public:
    explicit Checking(int balance) : Account(balance) {}

private:
    std::string do_withdraw(int amount) override {
        if (!take(amount)) {
            return "余额不足，未取出 " + std::to_string(amount);
        }
        return "活期取出 " + std::to_string(amount);
    }
};

class Savings : public Account {
public:
    explicit Savings(int balance) : Account(balance) {}

private:
    std::string do_withdraw(int amount) override {
        constexpr int fee = 2;
        if (!take(amount + fee)) {
            return "余额不足（含手续费），未取出 " + std::to_string(amount);
        }
        return "定期取出 " + std::to_string(amount) + "，手续费 " + std::to_string(fee);
    }
};

int main() {
    Checking checking(100);
    Savings savings(100);

    std::cout << checking.withdraw(30) << "，余额 " << checking.balance() << '\n';
    std::cout << savings.withdraw(30) << "，余额 " << savings.balance() << '\n';
    std::cout << checking.withdraw(-1) << "，余额 " << checking.balance() << '\n';
    std::cout << savings.withdraw(1000) << "，余额 " << savings.balance() << '\n';
}
