// mode: run
// 同一个计数器分别按值、std::ref 和指针传给线程。
#include <cstdio>
#include <functional>
#include <thread>

namespace {

struct Counter {
    long value;
};

void add_by_value(Counter counter) {
    for (int i = 0; i < 100000; ++i) {
        ++counter.value;
    }
}

void add_by_ref(Counter& counter) {
    for (int i = 0; i < 100000; ++i) {
        ++counter.value;
    }
}

void add_by_pointer(Counter* counter) {
    for (int i = 0; i < 100000; ++i) {
        ++counter->value;
    }
}

}  // namespace

int main() {
    Counter by_value{0};
    Counter by_ref{0};
    Counter by_pointer{0};

    std::thread t1(add_by_value, by_value);
    std::thread t2(add_by_ref, std::ref(by_ref));
    std::thread t3(add_by_pointer, &by_pointer);

    t1.join();
    t2.join();
    t3.join();

    std::printf("按值传：原对象 = %ld\n", by_value.value);
    std::printf("std::ref：原对象 = %ld\n", by_ref.value);
    std::printf("指针：原对象 = %ld\n", by_pointer.value);
    return 0;
}
