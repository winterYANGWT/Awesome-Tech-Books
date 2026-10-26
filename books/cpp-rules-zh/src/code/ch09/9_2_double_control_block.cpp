// 第 9.2 节：同一个裸指针构造两个 shared_ptr，会得到两个控制块和两次 delete
// mode: crash
// expect: AddressSanitizer
#include <iostream>
#include <memory>

struct Session {
    int id;
    ~Session() { std::cout << "  析构 Session " << id << "\n"; }
};

int main() {
    auto* raw = new Session{42};

    std::shared_ptr<Session> first(raw);
    std::shared_ptr<Session> second(raw);   // 又建了一个控制块

    std::cout << "first 的引用计数：" << first.use_count() << "\n";
    std::cout << "second 的引用计数：" << second.use_count() << "\n";
    std::cout << "两个计数各自归零时，*raw 会被销毁两次\n";
}
