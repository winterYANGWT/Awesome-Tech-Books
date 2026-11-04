// mode: run
#include <atomic>
#include <cstdio>
#include <type_traits>

// C++20 起：块作用域的 atomic<int> a; 值初始化成 0。
// C++17 及以前这里是不确定值，旧写法要写 ATOMIC_VAR_INIT(0)。
// atomic_flag 也默认 clear；旧写法是 = ATOMIC_FLAG_INIT。

int main() {
    std::atomic<int> a;
    std::atomic_flag f;
    static_assert(!std::is_copy_constructible_v<std::atomic<int>>);
    static_assert(!std::is_copy_assignable_v<std::atomic<int>>);

    std::printf("atomic<int> default = %d\n", a.load());
    std::printf("atomic_flag default test = %d\n", f.test());

    bool was_set = f.test_and_set();
    std::printf("test_and_set returns %d, test = %d\n", was_set, f.test());
    f.clear();
    std::printf("after clear test = %d\n", f.test());

    std::atomic<int> b{7};
    std::printf("atomic<int>{7} = %d\n", b.load());
}
