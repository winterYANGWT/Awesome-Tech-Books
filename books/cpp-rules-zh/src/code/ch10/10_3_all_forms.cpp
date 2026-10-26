// 第 10.3 节：类级 new/delete 要成套提供普通、placement、nothrow 和数组版本
#include <cstddef>
#include <iostream>
#include <new>

class Row {
public:
    Row() : value_(0) { ++constructed_; }
    explicit Row(int value) : value_(value) { ++constructed_; }
    ~Row() { ++destroyed_; }

    static void* operator new(std::size_t size) {
        ++plain_new_;
        return ::operator new(size);
    }
    static void operator delete(void* p) noexcept {
        ++plain_delete_;
        ::operator delete(p);
    }
    static void* operator new(std::size_t size, void* where) noexcept {
        ++placement_new_;
        return ::operator new(size, where);
    }
    static void operator delete(void* p, void* where) noexcept {
        ++placement_delete_;
        ::operator delete(p, where);
    }
    static void* operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
        ++nothrow_new_;
        return ::operator new(size, tag);
    }
    static void operator delete(void* p, const std::nothrow_t&) noexcept {
        ++nothrow_delete_;
        ::operator delete(p);
    }
    static void* operator new[](std::size_t size) {
        ++array_new_;
        return ::operator new[](size);
    }
    static void operator delete[](void* p) noexcept {
        ++array_delete_;
        ::operator delete[](p);
    }

    int value() const { return value_; }

    static void report() {
        std::cout << "  普通 new/delete：" << plain_new_ << "/" << plain_delete_ << "\n"
                  << "  placement new/delete：" << placement_new_ << "/" << placement_delete_ << "\n"
                  << "  nothrow new/delete：" << nothrow_new_ << "/" << nothrow_delete_ << "\n"
                  << "  数组 new[]/delete[]：" << array_new_ << "/" << array_delete_ << "\n"
                  << "  构造/析构：" << constructed_ << "/" << destroyed_ << "\n";
    }

private:
    int value_;
    inline static int plain_new_ = 0;
    inline static int plain_delete_ = 0;
    inline static int placement_new_ = 0;
    inline static int placement_delete_ = 0;
    inline static int nothrow_new_ = 0;
    inline static int nothrow_delete_ = 0;
    inline static int array_new_ = 0;
    inline static int array_delete_ = 0;
    inline static int constructed_ = 0;
    inline static int destroyed_ = 0;
};

int main() {
    std::cout << "1) 普通 new/delete\n";
    Row* a = new Row(1);
    std::cout << "  值：" << a->value() << "\n";
    delete a;

    std::cout << "2) placement new，显式析构\n";
    alignas(Row) std::byte buffer[sizeof(Row)];
    Row* b = new (buffer) Row(2);
    b->~Row();

    std::cout << "3) nothrow new；delete 仍走普通 delete\n";
    Row* c = new (std::nothrow) Row(3);
    delete c;

    std::cout << "4) 数组 new[]/delete[]\n";
    Row* d = new Row[3];
    delete[] d;

    Row::report();
}
