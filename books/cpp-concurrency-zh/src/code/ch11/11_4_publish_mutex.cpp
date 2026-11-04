// mode: run
// expect: data = 42
// data 和 ready 都在同一把锁下读写：unlock 与下一次 lock 同步。
#include <iostream>
#include <mutex>
#include <thread>

int main() {
    int data = 0;
    bool ready = false;
    std::mutex m;

    std::thread writer([&] {
        std::lock_guard<std::mutex> lock(m);
        data = 42;
        ready = true;
    });

    int seen = 0;
    std::thread reader([&] {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(m);
                if (ready) {
                    seen = data;
                    break;
                }
            }
            std::this_thread::yield();
        }
        std::cout << "data = " << seen << '\n';
    });

    writer.join();
    reader.join();
}
