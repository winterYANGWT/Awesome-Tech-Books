// mode: run
// expect: CAS\(\+0\.0 -> 1\.0\) = false
#include <atomic>
#include <iostream>

int main() {
    std::atomic<double> value{1.5};
    const double old = value.fetch_add(0.5);  // C++20 起浮点原子有 fetch_add
    std::cout << "fetch_add: 旧值 " << old << "，新值 " << value.load() << '\n';

    std::atomic<double> zero{-0.0};
    double expected = 0.0;  // 数学上 +0.0 == -0.0
    const bool ok = zero.compare_exchange_strong(expected, 1.0);
    std::cout << std::boolalpha;
    std::cout << "CAS(+0.0 -> 1.0) = " << ok << '\n';
    std::cout << "失败后 expected = " << expected << '\n';
}
