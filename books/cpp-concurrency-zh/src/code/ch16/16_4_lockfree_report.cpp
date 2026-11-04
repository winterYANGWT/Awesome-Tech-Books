// mode: run
// flags: -latomic
// 打印 16 字节计数指针的原子操作是否无锁，以及 C++26 两个头文件是否存在。
#include <atomic>
#include <cstdio>
#include <memory>

struct Node {
    int value;
};

struct counted_node_ptr {
    int external_count;
    Node* ptr;
};

int main() {
    std::atomic<counted_node_ptr> counted{};
    std::atomic<std::shared_ptr<int>> shared{};

    std::printf("sizeof(counted_node_ptr) = %zu\n", sizeof(counted_node_ptr));
    std::printf("atomic<counted_node_ptr>::is_lock_free() = %d\n",
                static_cast<int>(counted.is_lock_free()));
    std::printf("atomic<counted_node_ptr>::is_always_lock_free = %d\n",
                static_cast<int>(std::atomic<counted_node_ptr>::is_always_lock_free));
    std::printf("atomic<shared_ptr<int>>::is_lock_free() = %d\n",
                static_cast<int>(shared.is_lock_free()));
    std::printf("atomic<shared_ptr<int>>::is_always_lock_free = %d\n",
                static_cast<int>(std::atomic<std::shared_ptr<int>>::is_always_lock_free));

#if defined(__clang__)
    std::printf("compiler = Clang\n");
#elif defined(__GNUC__)
    std::printf("compiler = GCC\n");
#else
    std::printf("compiler = unknown\n");
#endif

#if defined(__has_include)
#  if __has_include(<hazard_pointer>)
    std::printf("<hazard_pointer> = 有\n");
#  else
    std::printf("<hazard_pointer> = 没有\n");
#  endif
#  if __has_include(<rcu>)
    std::printf("<rcu> = 有\n");
#  else
    std::printf("<rcu> = 没有\n");
#  endif
#endif

    std::printf("对照：clang++ -std=c++20 -mcx16 … -latomic\n");
    return 0;
}
