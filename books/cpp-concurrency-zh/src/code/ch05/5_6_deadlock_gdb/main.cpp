// 两个工作线程互相 join，主线程再 join 其中一个。
// 三个线程都停在 join 上，程序不会自己退出。
#include <iostream>
#include <latch>
#include <thread>

std::thread t1, t2;
std::latch go(1);

void f1() {
    go.wait();
    std::cout << "t1：join t2\n" << std::flush;
    t2.join();
}

void f2() {
    go.wait();
    std::cout << "t2：join t1\n" << std::flush;
    t1.join();
}

int main() {
    t1 = std::thread(f1);
    t2 = std::thread(f2);
    go.count_down();  // 两个 thread 对象都已构造好，才让它们互相 join
    std::cout << "main：join t1\n" << std::flush;
    t1.join();
}
