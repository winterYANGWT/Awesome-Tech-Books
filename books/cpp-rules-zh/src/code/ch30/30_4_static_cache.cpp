// 第 30.4 节：局部 static 的初始化线程安全，之后访问和返回地址都不安全
// flags: -pthread
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class FibCache {
public:
    FibCache() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "FibCache 构造（只会发生一次）\n";
    }
    std::vector<long long> values;
    std::mutex m;
};

// 第一次调用才构造 cache；C++11 起这件事由实现保证只做一次且线程安全。
// 但构造完成后的 push_back 不在“初始化”的保护范围内，仍要自己加锁。
const std::vector<long long>* fib(int size) {
    static FibCache cache;
    std::lock_guard<std::mutex> guard(cache.m);
    for (int i = static_cast<int>(cache.values.size()); i < size; ++i) {
        if (i < 2) cache.values.push_back(1);
        else cache.values.push_back(cache.values[i - 1] + cache.values[i - 2]);
    }
    return &cache.values;
}

int main() {
    std::cout << std::boolalpha;
    {
        // 两个线程同时触发第一次调用：初始化只做一次，另一个线程等它完成。
        std::thread a([] { fib(50); });
        std::thread b([] { fib(50); });
        a.join();
        b.join();
    }
    std::cout << "50 项已缓存: " << fib(50)->size() << '\n';

    const long long* old = fib(50)->data();
    fib(90);                                // 扩容会搬走缓冲区
    std::cout << "扩容后旧指针仍指向缓存: " << (old == fib(90)->data()) << '\n';
    std::cout << "90 项已缓存: " << fib(90)->size() << '\n';

    // 要长期保存就返回快照，而不是指向静态缓存的指针。
    const std::vector<long long> snapshot = *fib(90);
    std::cout << "快照大小: " << snapshot.size() << '\n';
}
