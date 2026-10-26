// 第 10.6 节：私有析构函数 + destroy() 要求对象只能由 new 创建
#include <iostream>

class Connection {
public:
    static Connection* open() { return new Connection; }

    // 伪析构函数：成员函数内部可以访问私有析构函数
    void destroy() const { delete this; }

    static int alive() { return alive_; }

private:
    Connection() { ++alive_; }
    ~Connection() { --alive_; std::cout << "  Connection 关闭\n"; }

    inline static int alive_ = 0;
};

int main() {
    std::cout << "1) 只能通过 Connection::open() 在堆上创建\n";
    Connection* a = Connection::open();
    Connection* b = Connection::open();
    std::cout << "  存活：" << Connection::alive() << "\n";

    std::cout << "2) 只能通过 destroy() 销毁，delete 在类外不可见\n";
    a->destroy();
    std::cout << "  关闭一个后存活：" << Connection::alive() << "\n";
    b->destroy();
    std::cout << "  全部关闭后存活：" << Connection::alive() << "\n";
}
