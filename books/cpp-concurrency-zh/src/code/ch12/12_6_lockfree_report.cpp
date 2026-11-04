// mode: run
// flags: -latomic
#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>

struct Pair {
    void* ptr;
    unsigned long tag;
};

template <typename T>
void report(const char* name) {
    std::atomic<T> value{};
    std::cout << std::boolalpha;
    std::cout << name << ": sizeof = " << sizeof(value)
              << "，is_always_lock_free = " << std::atomic<T>::is_always_lock_free
              << "，is_lock_free() = " << value.is_lock_free() << '\n';
}

int main() {
    report<bool>("bool");
    report<int>("int");
    report<void*>("void*");
    report<Pair>("Pair（16 字节）");
    report<std::shared_ptr<int>>("shared_ptr<int>");
}
