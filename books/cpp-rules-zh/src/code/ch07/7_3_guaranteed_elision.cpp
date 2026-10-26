// 第 7.3 节：C++17 保证纯右值的复制消除，删掉拷贝/移动也能返回
#include <iostream>
#include <type_traits>

struct Ticket {
    static int constructed;
    int id;

    explicit Ticket(int value) : id(value) { ++constructed; }
    Ticket(const Ticket&) = delete;
    Ticket(Ticket&&) = delete;
};
int Ticket::constructed = 0;

Ticket make_ticket() {
    return Ticket(7);  // 纯右值，直接在调用方的目标位置构造
}

void consume(Ticket ticket) {
    std::cout << "consume " << ticket.id << "\n";
}

int main() {
    static_assert(!std::is_copy_constructible_v<Ticket>);
    static_assert(!std::is_move_constructible_v<Ticket>);

    Ticket first = make_ticket();
    consume(make_ticket());
    Ticket second = Ticket(9);

    std::cout << "共构造 " << Ticket::constructed << " 个 Ticket，"
              << first.id << " / " << second.id << "\n";
}
