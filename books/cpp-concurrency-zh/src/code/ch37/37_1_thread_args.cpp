// mode: run
// output: varies
// expect: 按值: worker=0x… main=0x… -> 不同
// expect: 按引用: worker=0x… main=0x… -> 相同
// expect: lambda: a=[01] b=[01] sum=1
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>

struct Pair {
    int a;
    int b;
};

struct Result {
    std::uintptr_t seen;  // 存成整数，线程返回后比较不会碰到悬空指针
    int sum;
};

// 按值：线程拿到的是 Pair 的副本，地址和 main 里的对象不同。
static void work_by_value(Pair p, Result *out) {
    out->seen = reinterpret_cast<std::uintptr_t>(&p);
    out->sum = p.a + p.b;
}

// 按引用：std::ref 让线程直接读 main 里的对象，地址相同。
static void work_by_ref(Pair &p, Result *out) {
    out->seen = reinterpret_cast<std::uintptr_t>(&p);
    out->sum = p.a + p.b;
}

static std::atomic<int> call_order{0};

static int get_num(const char *tag) {
    int n = call_order.fetch_add(1);
    std::cout << "求值 " << tag << "：第 " << (n + 1) << " 个\n";
    return n;
}

int main() {
    Pair p{3, 4};
    Result by_value{};
    Result by_ref{};

    std::thread t1(work_by_value, p, &by_value);
    std::thread t2(work_by_ref, std::ref(p), &by_ref);
    t1.join();
    t2.join();

    const auto main_addr = reinterpret_cast<std::uintptr_t>(&p);
    std::cout << std::hex << "按值: worker=0x" << by_value.seen << " main=0x"
              << main_addr << std::dec << " -> "
              << (by_value.seen == main_addr ? "相同" : "不同") << "\n";
    std::cout << "按值: sum=" << by_value.sum << "\n";
    std::cout << std::hex << "按引用: worker=0x" << by_ref.seen << " main=0x"
              << main_addr << std::dec << " -> "
              << (by_ref.seen == main_addr ? "相同" : "不同") << "\n";
    std::cout << "按引用: sum=" << by_ref.sum << "\n";

    // 两个实参的求值顺序不定：先求出两个 int，再交给线程构造函数。
    std::thread t3([](int a, int b) {
        std::cout << "lambda: a=" << a << " b=" << b << " sum=" << (a + b) << "\n";
    }, get_num("A"), get_num("B"));
    t3.join();
    return 0;
}
