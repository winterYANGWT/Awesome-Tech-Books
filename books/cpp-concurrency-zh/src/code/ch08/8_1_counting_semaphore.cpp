// mode: run
// output: varies
// expect: 结束时空闲槽位: 4
// 4 个槽位的计数信号量：6 个生产者申请槽位，前 4 个写入，后 2 个阻塞在 acquire。
#include <semaphore>

#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kSlots = 4;
constexpr int kItems = 6;

std::counting_semaphore<kSlots> free_slots{kSlots};  // 空闲槽位
std::counting_semaphore<kItems> filled{0};           // 已写入的项
std::mutex table_mutex;                              // 保护 buffer 和计数
std::mutex io_mutex;                                 // 让整行输出不交错
int buffer[kSlots] = {-1, -1, -1, -1};
int written = 0;
int taken = 0;
int occupied = 0;  // 已占用的槽位，和 free_slots 的计数互补

void producer(int id) {
    free_slots.acquire();  // 槽位为 0 时在这里阻塞，不靠 try_acquire 判断
    int slot;
    int total;
    {
        std::lock_guard lk(table_mutex);
        for (slot = 0; slot < kSlots && buffer[slot] >= 0; ++slot) {
        }
        buffer[slot] = id;
        total = ++written;
        ++occupied;
        // 持表锁打印，主线程不会在 4 行写完之前看到 occupied == kSlots。
        std::lock_guard io(io_mutex);
        std::printf("生产者 %d: 写入槽 %d，已写入 %d 项\n", id, slot, total);
    }
    filled.release();
}

int take_one() {
    filled.acquire();
    int value;
    {
        std::lock_guard lk(table_mutex);
        int slot = 0;
        while (buffer[slot] < 0) ++slot;
        value = buffer[slot];
        buffer[slot] = -1;
        ++taken;
        --occupied;
    }
    free_slots.release();
    return value;
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < kItems; ++i) {
        threads.emplace_back(producer, i);
    }

    // 等 4 个槽位都写满，此时 free_slots 的计数必为 0，其余生产者只能在 acquire 上等。
    for (;;) {
        {
            std::lock_guard lk(table_mutex);
            if (occupied == kSlots) break;
        }
        std::this_thread::yield();
    }
    std::printf("主线程: 4 个槽位都被占用，其余生产者在 acquire 上等\n");

    for (int k = 0; k < kItems; ++k) {
        int value = take_one();
        std::printf("主线程: 取出生产者 %d 的项，归还一个槽位\n", value);
    }
    for (auto& t : threads) t.join();

    int left;
    {
        std::lock_guard lk(table_mutex);
        left = kSlots - occupied;
    }
    std::printf("写入 %d 项，取出 %d 项\n", written, taken);
    std::printf("结束时空闲槽位: %d\n", left);
}
