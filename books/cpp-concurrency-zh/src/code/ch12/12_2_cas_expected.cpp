// mode: run
// expect: expected = 42
#include <atomic>
#include <iostream>
#include <thread>

int main() {
    std::atomic<int> value{7};

    // 先让另一个线程把值从 7 改成 42，CAS 一定会失败
    std::thread other([&] { value.store(42); });
    other.join();

    int expected = 7;
    const bool ok = value.compare_exchange_strong(expected, 9);
    std::cout << std::boolalpha;
    std::cout << "CAS 成功? " << ok << '\n';
    std::cout << "expected = " << expected << '\n';
    std::cout << "value = " << value.load() << '\n';
}
