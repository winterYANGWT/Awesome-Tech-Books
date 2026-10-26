// 第 10.2 节：替换全局 operator new/delete，把每次分配记进账本
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
std::size_t allocation_count = 0;
std::size_t free_count = 0;
std::size_t requested_bytes = 0;

void* allocate(std::size_t size) {
    // 0 字节也要拿到合法指针，标准要求分配函数处理这种请求
    if (size == 0) size = 1;
    for (;;) {
        void* p = std::malloc(size);
        if (p != nullptr) {
            ++allocation_count;
            requested_bytes += size;
            return p;
        }
        // 分配失败：按 EC-51 的约定调用当前 new-handler，直到成功或它抛出
        std::new_handler handler = std::get_new_handler();
        if (handler == nullptr) throw std::bad_alloc();
        handler();
    }
}

void release(void* p) noexcept {
    if (p == nullptr) return;   // 释放函数必须接受空指针
    ++free_count;
    std::free(p);
}
} // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }

struct Order {
    int id;
    char note[24];
    ~Order() { note[0] = '\0'; }   // 非平凡析构，数组分配通常要加 cookie
};

int main() {
    std::size_t a0 = allocation_count;
    std::size_t f0 = free_count;
    std::size_t b0 = requested_bytes;
    {
        Order* one = new Order{1, "single"};
        delete one;
    }
    std::printf("单个对象：分配 %zu 次，释放 %zu 次，请求 %zu 字节\n",
                allocation_count - a0, free_count - f0, requested_bytes - b0);

    std::size_t a1 = allocation_count;
    std::size_t f1 = free_count;
    std::size_t b1 = requested_bytes;
    {
        Order* many = new Order[3];
        delete[] many;
    }
    std::printf("数组对象：分配 %zu 次，释放 %zu 次，请求 %zu 字节\n",
                allocation_count - a1, free_count - f1, requested_bytes - b1);
    std::printf("两次都配平：%s\n", allocation_count == free_count ? "是" : "否");
}
