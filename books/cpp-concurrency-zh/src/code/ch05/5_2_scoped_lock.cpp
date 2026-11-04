// mode: run
// output: varies
// expect: 总余额 2000000
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

struct Account {
    long long balance;
    std::mutex m;
};

// scoped_lock 一次锁两把，由实现选择不会死锁的顺序。
void transfer(Account& from, Account& to, long long amount) {
    std::scoped_lock lock(from.m, to.m);
    from.balance -= amount;
    to.balance += amount;
}

int main() {
    Account a{1'000'000, {}};
    Account b{1'000'000, {}};
    constexpr int rounds = 500'000;

    auto start = std::chrono::steady_clock::now();
    std::thread t1([&] {
        for (int i = 0; i < rounds; ++i) transfer(a, b, 1);
    });
    std::thread t2([&] {
        for (int i = 0; i < rounds; ++i) transfer(b, a, 1);
    });
    t1.join();
    t2.join();
    auto stop = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = stop - start;
    std::cout << "总余额 " << a.balance + b.balance << "\n";
    std::cout << "耗时 " << elapsed.count() << " ms\n";
}
