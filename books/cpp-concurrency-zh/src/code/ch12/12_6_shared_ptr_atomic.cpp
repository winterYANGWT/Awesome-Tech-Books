// mode: run
// flags: -latomic
#include <atomic>
#include <iostream>
#include <memory>

int main() {
    auto first = std::make_shared<int>(1);
    auto second = std::make_shared<int>(2);
    std::atomic<std::shared_ptr<int>> slot{first};

    std::shared_ptr<int> old = slot.exchange(second);
    std::cout << "交换出的旧值 = " << *old << '\n';
    std::cout << "现在的值 = " << *slot.load() << '\n';
    std::cout << std::boolalpha;
    std::cout << "is_lock_free() = " << slot.is_lock_free() << '\n';
}
