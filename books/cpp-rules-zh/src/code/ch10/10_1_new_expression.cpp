// 第 10.1 节：new 表达式分别调用 operator new 和构造函数，delete 反过来
#include <cstddef>
#include <iostream>
#include <new>
#include <string>

class Account {
public:
    Account(std::string owner, int balance) : owner_(std::move(owner)), balance_(balance) {
        std::cout << "    构造 " << owner_ << "，余额 " << balance_ << "\n";
    }
    ~Account() { std::cout << "    析构 " << owner_ << "\n"; }

    // 类级分配函数：只拿原始内存，不构造对象
    static void* operator new(std::size_t size) {
        ++allocations_;
        std::cout << "    Account::operator new(" << size << ")\n";
        return ::operator new(size);
    }
    static void operator delete(void* p) noexcept {
        ++frees_;
        std::cout << "    Account::operator delete\n";
        ::operator delete(p);
    }
    // placement 版本：把调用方给的地址原样返回
    static void* operator new(std::size_t size, void* where) noexcept {
        std::cout << "    placement operator new(" << size << ") -> 给定地址\n";
        return ::operator new(size, where);
    }
    static void operator delete(void* p, void* where) noexcept {
        std::cout << "    placement operator delete（只在构造抛异常时调用）\n";
        ::operator delete(p, where);
    }

    static int allocations() { return allocations_; }
    static int frees() { return frees_; }

private:
    std::string owner_;
    int balance_;
    inline static int allocations_ = 0;
    inline static int frees_ = 0;
};

class Base {
public:
    static void* operator new(std::size_t size) {
        std::cout << "    Base::operator new 收到 " << size << " 字节\n";
        return ::operator new(size);
    }
    static void operator delete(void* p) noexcept { ::operator delete(p); }
    int base_value = 1;
};

class Derived : public Base {
public:
    long long payload[4]{};   // 派生类更大，Base::operator new 会收到 sizeof(Derived)
};

int main() {
    std::cout << "1) new Account：先分配，再构造\n";
    Account* a = new Account("alice", 100);
    std::cout << "2) delete a：先析构，再释放\n";
    delete a;

    std::cout << "3) 直接调用 operator new：只有内存，没有对象\n";
    void* raw = ::operator new(sizeof(Account));
    std::cout << "    raw = " << (raw != nullptr ? "非空" : "空") << "\n";
    ::operator delete(raw);

    std::cout << "4) placement new：在栈上的缓冲区里构造，不分配内存\n";
    alignas(Account) std::byte buffer[sizeof(Account)];
    Account* b = new (buffer) Account("bob", 50);
    b->~Account();   // 必须显式析构；不能用 delete

    std::cout << "5) 派生类的 new 会调用基类的 operator new，但尺寸是派生类的\n";
    std::cout << "    sizeof(Base)=" << sizeof(Base)
              << "，sizeof(Derived)=" << sizeof(Derived) << "\n";
    Derived* d = new Derived;
    delete d;

    std::cout << "6) 请求 0 字节也必须拿到合法指针\n";
    void* zero = ::operator new(0);
    std::cout << "    zero = " << (zero != nullptr ? "非空" : "空") << "\n";
    ::operator delete(zero);

    std::cout << "Account 的分配/释放次数：" << Account::allocations()
              << "/" << Account::frees() << "\n";
}
