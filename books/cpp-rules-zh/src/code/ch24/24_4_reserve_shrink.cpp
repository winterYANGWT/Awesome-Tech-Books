// 第 24.4 节：reserve 只加容量不减元素，缩容要 shrink_to_fit 或交换
#include <cstddef>
#include <iostream>
#include <vector>

struct Ticket {
    int id;
    static inline int copies = 0;
    static inline int moves = 0;

    explicit Ticket(int i) : id(i) {}
    Ticket(const Ticket& other) : id(other.id) { ++copies; }
    Ticket(Ticket&& other) noexcept : id(other.id) { ++moves; }
    Ticket& operator=(const Ticket&) = default;
    Ticket& operator=(Ticket&&) = default;
};

std::vector<Ticket> build(bool reserve_first) {
    std::vector<Ticket> tickets;
    if (reserve_first) {
        tickets.reserve(1000);  // 只分配存储，不构造 Ticket
    }

    Ticket::copies = Ticket::moves = 0;
    std::size_t reallocations = 0;
    std::size_t capacity = tickets.capacity();
    std::vector<std::size_t> growth;
    for (int i = 0; i < 1000; ++i) {
        tickets.emplace_back(i);
        if (tickets.capacity() != capacity) {
            ++reallocations;
            capacity = tickets.capacity();
            growth.push_back(capacity);
        }
    }

    std::cout << (reserve_first ? "先 reserve(1000)" : "不 reserve")
              << ": 扩容 " << reallocations << " 次，拷贝 " << Ticket::copies
              << " 次，搬移 " << Ticket::moves << " 次，容量变化";
    if (growth.empty()) {
        std::cout << "（无）";
    } else {
        for (std::size_t c : growth) {
            std::cout << ' ' << c;
        }
    }
    std::cout << "，最终 capacity " << tickets.capacity() << '\n';
    return tickets;
}

int main() {
    auto without = build(false);
    auto with = build(true);

    std::cout << "reserve 之后 size 仍是 " << with.size() << '\n';

    // erase 只改 size，不还内存
    with.erase(with.begin() + 10, with.end());
    std::cout << "erase 到 10 条后: size " << with.size()
              << "，capacity " << with.capacity() << '\n';

    // reserve 小于当前容量时什么也不做，绝不会缩容
    with.reserve(10);
    std::cout << "再 reserve(10) 后 capacity " << with.capacity() << '\n';

    // C++11 的 shrink_to_fit 是请求式的，标准不保证容量正好等于 size
    with.shrink_to_fit();
    std::cout << "shrink_to_fit 后: size " << with.size()
              << "，capacity " << with.capacity() << '\n';

    // 交换技巧：拷贝出只含现有元素的临时容器，再交换，临时对象带走旧缓冲区
    std::vector<Ticket> oversized;
    oversized.reserve(500);
    for (int i = 0; i < 10; ++i) {
        oversized.emplace_back(i);
    }
    std::cout << "交换前: size " << oversized.size()
              << "，capacity " << oversized.capacity() << '\n';
    std::vector<Ticket>(oversized).swap(oversized);
    std::cout << "交换后: size " << oversized.size()
              << "，capacity " << oversized.capacity() << '\n';

    // clear 只销毁元素，容量保留
    std::vector<Ticket> reusable;
    reusable.reserve(64);
    for (int i = 0; i < 5; ++i) {
        reusable.emplace_back(i);
    }
    const auto capacity_before_clear = reusable.capacity();
    reusable.clear();
    std::cout << "clear 后: size " << reusable.size()
              << "，容量未变 " << (reusable.capacity() == capacity_before_clear) << '\n';
}
