// 第 10.6 节：用私有析构禁止栈对象，用删除的 operator new 禁止堆对象
// mode: fail
// expect: private
#include <cstddef>

class Connection {
public:
    Connection() = default;
    void destroy() const { delete this; }

private:
    ~Connection() = default;
};

class StackOnly {
public:
    StackOnly() = default;
    static void* operator new(std::size_t) = delete;
    static void operator delete(void*) = delete;
};

int main() {
    Connection local;               // 错误：离开作用域时析构函数不可访问
    Connection* p = new Connection;
    delete p;                       // 错误：析构函数私有
    StackOnly* s = new StackOnly;   // 错误：operator new 被删除
}
