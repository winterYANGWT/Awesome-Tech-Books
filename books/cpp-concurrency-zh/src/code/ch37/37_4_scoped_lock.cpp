// mode: run
// expect: 转账后: A=1000 B=1000 总额=2000
// expect: 读写锁: value=100 reads=4000 negative=0
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

struct Account {
    std::mutex m;
    long balance;
    explicit Account(long b) : balance(b) {}
};

// C++17：scoped_lock 一次锁两个，内部用避免死锁的算法。
static void transfer(Account &from, Account &to, int times) {
    for (int i = 0; i < times; ++i) {
        std::scoped_lock lock(from.m, to.m);
        if (from.balance > 0) {
            --from.balance;
            ++to.balance;
        }
    }
}

struct Counter {
    std::shared_mutex m;
    long value = 0;
};

static void reader(Counter &c, int times, long *reads) {
    long n = 0;
    for (int i = 0; i < times; ++i) {
        std::shared_lock<std::shared_mutex> lock(c.m);
        if (c.value < 0) {
            std::terminate();  // 读到的值不应为负；写者只在 0..100 之间改
        }
        ++n;
    }
    *reads = n;
}

static void writer(Counter &c, int times) {
    for (int i = 0; i < times; ++i) {
        std::unique_lock<std::shared_mutex> lock(c.m);
        ++c.value;
    }
}

int main() {
    Account a(1000);
    Account b(1000);
    std::thread t1(transfer, std::ref(a), std::ref(b), 1000);
    std::thread t2(transfer, std::ref(b), std::ref(a), 1000);
    t1.join();
    t2.join();
    std::cout << "转账后: A=" << a.balance << " B=" << b.balance
              << " 总额=" << (a.balance + b.balance) << "\n";

    Counter c;
    constexpr int kReaders = 4;
    constexpr int kReads = 1000;
    std::vector<long> reads(kReaders, 0);
    std::thread w(writer, std::ref(c), 100);
    std::vector<std::thread> readers;
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back(reader, std::ref(c), kReads, &reads[i]);
    }
    w.join();
    for (auto &t : readers) {
        t.join();
    }
    long total_reads = 0;
    for (long n : reads) {
        total_reads += n;
    }
    std::cout << "读写锁: value=" << c.value << " reads=" << total_reads
              << " negative=0\n";
    return 0;
}
