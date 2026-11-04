// mode: run
// 用一个 mutex 保护状态，两个条件变量分别等“可读”和“可写”；写者优先。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

class RwLock {
public:
    void lock_shared() {
        std::unique_lock<std::mutex> lock(mutex_);
        can_read_.wait(lock, [this] {
            return !writer_active_ && waiting_writers_ == 0;  // 有写者排队时，新读者也等
        });
        ++active_readers_;
    }

    void unlock_shared() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (--active_readers_ == 0) can_write_.notify_one();
    }

    void lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++waiting_writers_;
        can_write_.wait(lock, [this] { return !writer_active_ && active_readers_ == 0; });
        --waiting_writers_;
        writer_active_ = true;
    }

    void unlock() {
        std::lock_guard<std::mutex> lock(mutex_);
        writer_active_ = false;
        if (waiting_writers_ > 0) {
            can_write_.notify_one();  // 先把写者放进去，避免写者饥饿
        } else {
            can_read_.notify_all();
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable can_read_;
    std::condition_variable can_write_;
    int active_readers_ = 0;
    int waiting_writers_ = 0;
    bool writer_active_ = false;
};

int main() {
    RwLock rw;

    // 1. 多个读者可以同时持有。
    std::atomic<int> inside{0};
    std::atomic<int> max_inside{0};
    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back([&] {
            rw.lock_shared();
            const int now = inside.fetch_add(1) + 1;
            int seen = max_inside.load();
            while (now > seen && !max_inside.compare_exchange_weak(seen, now)) {}
            std::this_thread::sleep_for(30ms);
            inside.fetch_sub(1);
            rw.unlock_shared();
        });
    }
    for (auto& thread : readers) thread.join();
    std::cout << "多读者并发：最多 " << max_inside.load() << " 个读者同时持有\n";

    // 2. 写者必须等所有读者退出。
    std::atomic<bool> writer_entered{false};
    std::atomic<bool> release_reader{false};
    std::thread reader([&] {
        rw.lock_shared();
        while (!release_reader.load()) std::this_thread::yield();
        rw.unlock_shared();
    });
    std::this_thread::sleep_for(10ms);
    std::thread writer([&] {
        rw.lock();
        writer_entered.store(true);
        std::this_thread::sleep_for(30ms);
        rw.unlock();
    });
    std::this_thread::sleep_for(20ms);
    const bool entered_early = writer_entered.load();
    release_reader.store(true);
    reader.join();
    writer.join();
    std::cout << "写者独占：读者持有时写者进入=" << (entered_early ? "是" : "否")
              << "，读者退出后写者进入=" << (writer_entered.load() ? "是" : "否") << '\n';

    // 3. 写者排队后，新读者不再插队。
    std::atomic<bool> release_first{false};
    std::atomic<bool> new_reader_entered{false};
    std::thread first([&] {
        rw.lock_shared();
        while (!release_first.load()) std::this_thread::yield();
        rw.unlock_shared();
    });
    std::this_thread::sleep_for(10ms);
    std::thread queued_writer([&] {
        rw.lock();
        std::this_thread::sleep_for(30ms);
        rw.unlock();
    });
    std::this_thread::sleep_for(10ms);
    std::thread second([&] {
        rw.lock_shared();
        new_reader_entered.store(true);
        rw.unlock_shared();
    });
    std::this_thread::sleep_for(10ms);
    const bool barged = new_reader_entered.load();
    release_first.store(true);
    first.join();
    queued_writer.join();
    second.join();
    std::cout << "写者优先：写者等待时新读者先进入=" << (barged ? "是" : "否") << '\n';
}
