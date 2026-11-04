// mode: run
// timeout: 30
// 用 release/acquire 传递 100 万次数据，统计读线程看到“标志已置位、数据仍旧”的次数。
#include <atomic>
#include <cstdio>
#include <thread>

namespace {

constexpr long kRounds = 1'000'000;

std::atomic<long> ready{0};   // 写线程发布：值 i+1 表示 data 已经写成 i
std::atomic<long> consumed{-1};
int data = 0;                 // 普通 int，靠 ready/consumed 的 release/acquire 保护

}  // namespace

int main() {
    long stale = 0;

    std::thread writer([] {
        for (long i = 0; i < kRounds; ++i) {
            data = static_cast<int>(i & 0x7fffffff);
            ready.store(i + 1, std::memory_order_release);
            while (consumed.load(std::memory_order_acquire) != i) {
            }
        }
    });

    std::thread reader([&stale] {
        for (long i = 0; i < kRounds; ++i) {
            while (ready.load(std::memory_order_acquire) != i + 1) {
            }
            if (data != static_cast<int>(i & 0x7fffffff)) {
                ++stale;
            }
            consumed.store(i, std::memory_order_release);
        }
    });

    writer.join();
    reader.join();

    std::printf("rounds=%ld\n", kRounds);
    std::printf("stale_reads=%ld\n", stale);
    std::printf("final_data=%d\n", data);
    return 0;
}
