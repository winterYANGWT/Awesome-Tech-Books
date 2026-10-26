// 第 11.4 节：new[] 表达式在某个元素构造失败时自动析构已完成的元素并释放内存
#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>

struct Widget {
    int id;

    Widget() : id(next_id_++) {
        if (id == 2) {
            throw std::runtime_error("第 2 个 Widget 构造失败");
        }
        ++live_;
        ++constructed_;
        std::cout << "  构造 Widget " << id << "（live=" << live_ << "）\n";
    }

    ~Widget() {
        --live_;
        ++destroyed_;
        std::cout << "  析构 Widget " << id << "（live=" << live_ << "）\n";
    }

    static void* operator new[](std::size_t bytes) {
        ++allocations_;
        std::cout << "  operator new[](" << bytes << " 字节)\n";
        return ::operator new[](bytes);
    }

    static void operator delete[](void* p) noexcept {
        ++deallocations_;
        std::cout << "  operator delete[]\n";
        ::operator delete[](p);
    }

    static inline int next_id_ = 0;
    static inline int live_ = 0;
    static inline int constructed_ = 0;
    static inline int destroyed_ = 0;
    static inline int allocations_ = 0;
    static inline int deallocations_ = 0;
};

int main() {
    try {
        Widget* array = new Widget[3];    // 第 3 个构造抛出，赋值不会发生
        (void)array;
    } catch (const std::exception& e) {
        std::cout << "捕获：" << e.what() << "\n";
    }

    std::cout << "constructed=" << Widget::constructed_
              << " destroyed=" << Widget::destroyed_
              << " live=" << Widget::live_
              << " allocations=" << Widget::allocations_
              << " deallocations=" << Widget::deallocations_ << "\n";
}
