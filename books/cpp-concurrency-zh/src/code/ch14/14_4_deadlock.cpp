// mode: hang
// expect: 仍未结束
// 两个线程按相反顺序锁两个桶；barrier 保证各自先拿到第一把，再一起要第二把。
#include <barrier>
#include <cstdio>
#include <mutex>
#include <shared_mutex>
#include <thread>

namespace {

struct Bucket {
    std::shared_mutex mutex;
};

Bucket g_buckets[2];
std::barrier g_both_ready{2};

void lock_pair(int first, int second, const char* name) {
    std::unique_lock<std::shared_mutex> a(g_buckets[first].mutex);
    std::printf("%s 锁住桶 %d\n", name, first);
    std::fflush(stdout);
    g_both_ready.arrive_and_wait();  // 两把第一锁都拿到，再要第二把
    std::unique_lock<std::shared_mutex> b(g_buckets[second].mutex);
    std::printf("%s 锁住桶 %d\n", name, second);
    std::fflush(stdout);
}

}  // namespace

int main() {
    std::thread a(lock_pair, 0, 1, "A");
    std::thread b(lock_pair, 1, 0, "B");
    a.join();
    b.join();
    return 0;
}
