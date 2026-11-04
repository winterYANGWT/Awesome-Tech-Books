// mode: run
// expect: 返回 1，两个 owns_lock 都是 0
#include <iostream>
#include <mutex>

int main() {
    std::mutex m1;
    std::mutex m2;

    std::unique_lock<std::mutex> a(m1, std::defer_lock);  // 先关联，不加锁
    std::unique_lock<std::mutex> b(m2, std::defer_lock);
    std::cout << "defer_lock 之后：owns_lock " << a.owns_lock() << ' ' << b.owns_lock() << '\n';

    std::lock(a, b);  // 一次锁住两个，内部按算法避免相反顺序
    std::cout << "std::lock 之后：owns_lock " << a.owns_lock() << ' ' << b.owns_lock() << '\n';

    a.unlock();  // unique_lock 可以手动解锁，不必等析构
    std::cout << "解锁第一个之后：owns_lock " << a.owns_lock() << ' ' << b.owns_lock() << '\n';

    std::unique_lock<std::mutex> c(std::move(b));  // 所有权转移，源不再持有
    std::cout << "移动之后：源 owns_lock " << b.owns_lock() << "，目标 owns_lock "
              << c.owns_lock() << '\n';
    c.unlock();

    a.lock();  // 手动再加锁，下面用 try_to_lock 撞它
    std::unique_lock<std::mutex> d(m1, std::try_to_lock);
    std::cout << "try_to_lock 抢已被占用的锁：owns_lock " << d.owns_lock() << '\n';
    a.unlock();

    std::mutex m3;
    std::mutex m4;
    std::unique_lock<std::mutex> l3(m3, std::defer_lock);
    std::unique_lock<std::mutex> l4(m4, std::defer_lock);
    const int all = std::try_lock(l3, l4);
    std::cout << "std::try_lock 全成功：返回 " << all << "，owns_lock "
              << l3.owns_lock() << ' ' << l4.owns_lock() << '\n';
    l3.unlock();
    l4.unlock();

    std::lock_guard<std::mutex> held(m3);  // 占住 m3，让第二个 try_lock 失败
    std::unique_lock<std::mutex> x(m4, std::defer_lock);
    std::unique_lock<std::mutex> y(m3, std::defer_lock);
    const int failed = std::try_lock(x, y);
    std::cout << "std::try_lock 第二个失败：返回 " << failed << "，两个 owns_lock 都是 "
              << x.owns_lock() << ' ' << y.owns_lock() << '\n';
}
