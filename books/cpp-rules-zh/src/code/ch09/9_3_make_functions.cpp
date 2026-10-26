// 第 9.3 节：make 函数少写类型，make_shared 还把对象和控制块放在一次分配里
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

// 统计全局分配次数。只用于示例：真实程序不要靠替换全局 new 做统计。
namespace {
std::size_t allocations = 0;
std::size_t deallocations = 0;
}  // namespace

void* operator new(std::size_t size) {
    ++allocations;
    if (void* p = std::malloc(size)) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    ++allocations;
    if (void* p = std::malloc(size)) {
        return p;
    }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {
    if (p) {
        ++deallocations;
        std::free(p);
    }
}
void operator delete[](void* p) noexcept {
    if (p) {
        ++deallocations;
        std::free(p);
    }
}
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { operator delete[](p); }

struct Widget {
    explicit Widget(int n) : id(n) { ++live; }
    ~Widget() { --live; }
    int id;
    static int live;
};
int Widget::live = 0;

struct Job {
    explicit Job(int n) : id(n) { ++live; }
    ~Job() { --live; }
    int id;
    static int live;
};
int Job::live = 0;

void process(std::unique_ptr<Job> job, int priority) {
    std::cout << "  处理 Job " << job->id << "，优先级 " << priority << "\n";
}

int failing_priority() {
    throw std::runtime_error("优先级服务不可用");
}

int main() {
    std::cout << "== 分配次数 ==\n";
    std::cout.flush();  // 先让 iostream 自己的内部分配发生完

    auto before = allocations;
    auto shared_make = std::make_shared<Widget>(1);
    auto make_cost = allocations - before;

    before = allocations;
    std::shared_ptr<Widget> shared_direct(new Widget(2));
    auto direct_cost = allocations - before;

    before = allocations;
    auto unique_make = std::make_unique<Widget>(3);
    auto unique_cost = allocations - before;

    std::cout << "make_shared 分配 " << make_cost << " 次\n";
    std::cout << "shared_ptr(new Widget) 分配 " << direct_cost << " 次\n";
    std::cout << "make_unique 分配 " << unique_cost << " 次\n";
    std::cout << "两个 shared_ptr 的引用计数：" << shared_make.use_count()
              << "、" << shared_direct.use_count()
              << "；unique_ptr 是否为空：" << std::boolalpha
              << (unique_make == nullptr) << "\n";

    std::cout << "== 异常路径 ==\n";
    try {
        // 两个实参的求值顺序不确定；无论谁先，临时 unique_ptr 都会清理 Job。
        process(std::make_unique<Job>(9), failing_priority());
    } catch (const std::exception& e) {
        std::cout << "  捕获：" << e.what() << "\n";
    }
    std::cout << "  优先级失败后存活 Job " << Job::live << " 个\n";
}
