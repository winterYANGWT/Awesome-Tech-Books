// 第 9.5 节：类级 new 要和参数匹配的 delete 成对，三种标准形式也要补回
#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>

class Arena {
public:
    void* allocate(std::size_t size) {
        ++allocations;
        return ::operator new(size);
    }
    void release(void* p) noexcept {
        ++releases;
        ::operator delete(p);
    }
    int allocations = 0;
    int releases = 0;
};

class Gadget {
public:
    explicit Gadget(bool fail) {
        if (fail) {
            throw std::runtime_error("Gadget 构造失败");
        }
    }
    ~Gadget() = default;

    // 自定义的池分配：额外参数是 Arena&。
    static void* operator new(std::size_t size, Arena& arena) {
        ++arena_new_calls;
        return arena.allocate(size);
    }
    // 配对的 placement delete：new 的参数表去掉 size_t、在最前面换成 void*。
    static void operator delete(void* p, Arena& arena) noexcept {
        ++arena_delete_calls;
        arena.release(p);
    }

    // 声明了上面那个 new，下面三种全局形式都会被隐藏，必须补回。
    static void* operator new(std::size_t size) {
        ++plain_new_calls;
        return ::operator new(size);
    }
    static void operator delete(void* p) noexcept {
        ++plain_delete_calls;
        ::operator delete(p);
    }

    static void* operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
        ++nothrow_new_calls;
        return ::operator new(size, tag);
    }
    static void operator delete(void* p, const std::nothrow_t&) noexcept {
        ++nothrow_delete_calls;
        ::operator delete(p);
    }

    static void* operator new(std::size_t, void* where) noexcept {
        ++placement_new_calls;
        return where;
    }
    static void operator delete(void*, void*) noexcept {
        ++placement_delete_calls;
    }

    static int arena_new_calls;
    static int arena_delete_calls;
    static int plain_new_calls;
    static int plain_delete_calls;
    static int nothrow_new_calls;
    static int nothrow_delete_calls;
    static int placement_new_calls;
    static int placement_delete_calls;
};

int Gadget::arena_new_calls = 0;
int Gadget::arena_delete_calls = 0;
int Gadget::plain_new_calls = 0;
int Gadget::plain_delete_calls = 0;
int Gadget::nothrow_new_calls = 0;
int Gadget::nothrow_delete_calls = 0;
int Gadget::placement_new_calls = 0;
int Gadget::placement_delete_calls = 0;

int main() {
    std::cout << "== 普通 new ==\n";
    Gadget* plain = new Gadget(false);
    delete plain;
    std::cout << "  普通 new/delete 累计：" << Gadget::plain_new_calls << "/"
              << Gadget::plain_delete_calls << "\n";

    std::cout << "== nothrow new ==\n";
    Gadget* safe = new (std::nothrow) Gadget(false);
    delete safe;
    std::cout << "  nothrow new 累计：" << Gadget::nothrow_new_calls
              << "；正常 delete 仍走普通版本，累计：" << Gadget::plain_delete_calls
              << "\n";

    std::cout << "== placement new ==\n";
    alignas(Gadget) unsigned char buffer[sizeof(Gadget)];
    Gadget* placed = new (buffer) Gadget(false);
    placed->~Gadget();
    std::cout << "  placement new 累计：" << Gadget::placement_new_calls
              << "；构造未抛，配对 delete 累计：" << Gadget::placement_delete_calls
              << "\n";

    std::cout << "== 自定义 Arena 分配，构造函数抛异常 ==\n";
    Arena arena;
    try {
        Gadget* doomed = new (arena) Gadget(true);
        (void)doomed;
    } catch (const std::exception& e) {
        std::cout << "  捕获：" << e.what() << "\n";
    }

    std::cout << "普通 new/delete 调用：" << Gadget::plain_new_calls << "/"
              << Gadget::plain_delete_calls << "\n";
    std::cout << "nothrow new/delete 调用：" << Gadget::nothrow_new_calls << "/"
              << Gadget::nothrow_delete_calls << "\n";
    std::cout << "placement new/delete 调用：" << Gadget::placement_new_calls << "/"
              << Gadget::placement_delete_calls << "\n";
    std::cout << "Arena new/delete 调用：" << Gadget::arena_new_calls << "/"
              << Gadget::arena_delete_calls << "\n";
    std::cout << "Arena 分配/释放：" << arena.allocations << "/"
              << arena.releases << "\n";
}
