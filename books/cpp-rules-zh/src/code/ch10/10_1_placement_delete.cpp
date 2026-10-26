// 第 10.1 节：placement new 的配套 delete 只在构造函数抛异常时被调用
#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>

// 一块自己记账的存储：take/give 只是转调全局 operator new/delete
class Arena {
public:
    void* take(std::size_t size) {
        ++blocks_;
        last_ = ::operator new(size);
        return last_;
    }
    void give(void* p) {
        if (p == nullptr) return;
        --blocks_;
        ::operator delete(p);
    }
    int blocks() const { return blocks_; }
    void* last() const { return last_; }

private:
    int blocks_ = 0;
    void* last_ = nullptr;
};

class WithDelete {
public:
    explicit WithDelete(bool fail) {
        if (fail) throw std::runtime_error("构造失败");
    }
    static void* operator new(std::size_t size, Arena& arena) { return arena.take(size); }
    static void operator delete(void* p, Arena& arena) noexcept { arena.give(p); }
};

class WithoutDelete {
public:
    explicit WithoutDelete(bool fail) {
        if (fail) throw std::runtime_error("构造失败");
    }
    static void* operator new(std::size_t size, Arena& arena) { return arena.take(size); }
    // 故意不写 operator delete(void*, Arena&)
};

int main() {
    Arena arena;

    std::cout << "1) 构造成功：placement delete 不会被任何 delete 表达式调用\n";
    WithDelete* ok = new (arena) WithDelete(false);
    ok->~WithDelete();
    arena.give(ok);   // 存储是调用方拿来的，必须调用方归还
    std::cout << "    未归还块数：" << arena.blocks() << "\n";

    std::cout << "2) 构造抛异常，签名匹配的 placement delete 被自动调用\n";
    try {
        new (arena) WithDelete(true);
    } catch (const std::exception& e) {
        std::cout << "    捕获：" << e.what() << "\n";
    }
    std::cout << "    未归还块数：" << arena.blocks() << "\n";

    std::cout << "3) 没有匹配的 placement delete：运行时不知道该怎么释放\n";
    try {
        new (arena) WithoutDelete(true);
    } catch (const std::exception& e) {
        std::cout << "    捕获：" << e.what() << "\n";
    }
    std::cout << "    未归还块数：" << arena.blocks() << "\n";
    arena.give(arena.last());   // 只能手动收拾
    std::cout << "    手动归还后：" << arena.blocks() << "\n";
}
