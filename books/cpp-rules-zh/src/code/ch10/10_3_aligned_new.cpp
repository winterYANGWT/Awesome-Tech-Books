// 第 10.3 节：过对齐类型要补对齐版 operator new/delete
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>

// 只写了普通版本：new 表达式找不到对齐版，就调用这个版本
struct alignas(64) Wide {
    int value = 0;
    static inline int plain_calls = 0;
    static inline int aligned_calls = 0;
    static void* operator new(std::size_t size) {
        ++plain_calls;
        return ::operator new(size);
    }
    static void operator delete(void* p) noexcept { ::operator delete(p); }
};

// 普通版本 + 对齐版本都写：new 表达式会优先选对齐版
struct alignas(64) WideFixed {
    int value = 0;
    static inline int plain_calls = 0;
    static inline int aligned_calls = 0;
    static void* operator new(std::size_t size, std::align_val_t align) {
        ++aligned_calls;
        return ::operator new(size, align);
    }
    static void operator delete(void* p, std::align_val_t align) noexcept {
        ::operator delete(p, align);
    }
    static void* operator new(std::size_t size) {
        ++plain_calls;
        return ::operator new(size, std::align_val_t{alignof(WideFixed)});
    }
    static void operator delete(void* p) noexcept {
        ::operator delete(p, std::align_val_t{alignof(WideFixed)});
    }
};

int main() {
    std::cout << "默认 new 对齐：" << static_cast<std::size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
              << "，alignof(Wide)=" << alignof(Wide) << "\n";

    std::cout << "1) 只有普通版本：直接调用它，拿到的是默认对齐的块\n";
    void* raw = Wide::operator new(sizeof(Wide));
    std::cout << "  plain=" << Wide::plain_calls << "，aligned=" << Wide::aligned_calls
              << "，块非空：" << (raw != nullptr ? "是" : "否") << "\n";
    Wide::operator delete(raw);

    std::cout << "2) 普通 + 对齐版本：new 表达式选中对齐版\n";
    WideFixed* p = new WideFixed;
    bool aligned = reinterpret_cast<std::uintptr_t>(p) % alignof(WideFixed) == 0;
    std::cout << "  plain=" << WideFixed::plain_calls
              << "，aligned=" << WideFixed::aligned_calls
              << "，地址满足 64 对齐：" << (aligned ? "是" : "否") << "\n";
    delete p;
}
