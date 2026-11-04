// mode: race
// expect: data race
// timeout: 10
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>

// 这是旧的双重检查锁。为了在 x86 上稳定看到“指针先可见”，
// 构造函数把 instance 的写提前到字段填完之前；真实编译器/CPU 也可能重排。
struct Singleton {
    int value;
    bool constructed;
    Singleton();
};

Singleton* instance = nullptr;
std::mutex init_mutex;
std::atomic<bool> reader_saw{false};

Singleton::Singleton() : value(0), constructed(false) {
    instance = this;  // 相当于把赋值提到构造完成之前
    while (!reader_saw.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    value = 42;
    constructed = true;
}

Singleton* get() {
    if (instance == nullptr) {                    // 第一次检查，锁外
        std::lock_guard<std::mutex> lock(init_mutex);
        if (instance == nullptr) {                // 第二次检查，锁内
            instance = new Singleton();
        }
    }
    return instance;
}

int main() {
    puts("双重检查锁：构造函数里先发布指针，再填字段");
    std::thread reader([] {
        while (instance == nullptr) {
            std::this_thread::yield();
        }
        if (!instance->constructed) {
            puts("reader: instance 非空，constructed 还是 false");
        }
        reader_saw.store(true, std::memory_order_release);
        while (!instance->constructed) {
            std::this_thread::yield();
        }
        std::printf("reader: value = %d\n", instance->value);
    });
    std::thread writer([] { (void)get(); });
    writer.join();
    reader.join();
    delete instance;
}
