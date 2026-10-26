// 第 8.2 节：有析构函数却依赖隐式拷贝时，编译器可以给出警告
// mode: warn
// flags: -Wdeprecated-copy-dtor
// expect: deprecated-copy
#include <iostream>

class Ticket {
public:
    explicit Ticket(int id) : id_(id) {}
    ~Ticket() { std::cout << "回收票 " << id_ << "\n"; }

    int id() const { return id_; }

    // 没有声明拷贝构造和拷贝赋值：编译器生成逐成员复制，
    // 两个 Ticket 会各自回收同一个 id。
private:
    int id_;
};

int main() {
    Ticket first(3);
    Ticket second = first;
    std::cout << "两张票的 id：" << first.id() << " 和 " << second.id() << "\n";
}
