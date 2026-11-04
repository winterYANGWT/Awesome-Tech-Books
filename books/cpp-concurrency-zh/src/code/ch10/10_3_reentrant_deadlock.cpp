// mode: hang
// flags: -fno-inline
// expect: std::mutex::lock|仍未结束
#include <cstdio>
#include <mutex>
#include <thread>

// 每个成员函数都加锁，所以“多线程同时调用”不会破坏不变量。
// 但 deposit 持锁调用 apply，apply 又去锁同一把非递归 mutex。
// ANCHOR: account
class Account {
public:
    void deposit(int amount) {
        std::lock_guard<std::mutex> lock(m_);
        apply(amount);
    }

private:
    void apply(int amount) {
        std::lock_guard<std::mutex> lock(m_);
        balance_ += amount;
    }

    std::mutex m_;
    int balance_ = 0;
};
// ANCHOR_END: account

int main() {
    Account account;
    std::thread t([&] { account.deposit(10); });
    t.join();
    std::printf("done\n");
}
