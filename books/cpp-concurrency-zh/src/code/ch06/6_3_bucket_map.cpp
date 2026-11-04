// mode: run
// 64 个桶，每个桶一把 shared_mutex；快照按桶下标 0..63 加锁，再逆序解锁。
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

constexpr int kBuckets = 64;
constexpr int kThreads = 8;
constexpr int kKeysPerThread = 512;
constexpr int kUpdates = 5;

struct Record {
    int generation = 0;
    int mirrored = 0;  // 必须等于 generation + key
};

struct alignas(64) Bucket {
    mutable std::shared_mutex mutex;
    std::unordered_map<int, Record> entries;
};

std::array<Bucket, kBuckets> g_buckets;

Bucket& bucket_for(int key) {
    return g_buckets[std::hash<int>{}(key) % kBuckets];
}

void insert_key(int key) {
    Bucket& bucket = bucket_for(key);
    std::unique_lock<std::shared_mutex> lock(bucket.mutex);
    bucket.entries[key] = Record{1, 1 + key};
}

bool update_key(int key) {
    Bucket& bucket = bucket_for(key);
    std::unique_lock<std::shared_mutex> lock(bucket.mutex);
    const auto it = bucket.entries.find(key);
    if (it == bucket.entries.end()) return false;
    ++it->second.generation;
    it->second.mirrored = it->second.generation + key;
    return true;
}

bool query_key(int key) {
    Bucket& bucket = bucket_for(key);
    std::shared_lock<std::shared_mutex> lock(bucket.mutex);
    return bucket.entries.find(key) != bucket.entries.end();
}

bool erase_key(int key) {
    Bucket& bucket = bucket_for(key);
    std::unique_lock<std::shared_mutex> lock(bucket.mutex);
    return bucket.entries.erase(key) != 0;
}

// std::scoped_lock 只能锁编译期已知个数的锁；运行期的桶数组要自己写下标循环。
class AllBuckets {
public:
    AllBuckets() {
        for (auto& bucket : g_buckets) bucket.mutex.lock();
    }
    ~AllBuckets() {
        for (auto it = g_buckets.rbegin(); it != g_buckets.rend(); ++it) it->mutex.unlock();
    }
    AllBuckets(const AllBuckets&) = delete;
    AllBuckets& operator=(const AllBuckets&) = delete;
};

std::vector<std::pair<int, Record>> snapshot(std::atomic<bool>& holding,
                                             std::atomic<bool>& release_snapshot) {
    std::vector<std::pair<int, Record>> rows;
    rows.reserve(kThreads * kKeysPerThread);
    AllBuckets all;  // 构造时按 0..63 加锁，析构时逆序解锁
    holding.store(true);
    while (!release_snapshot.load()) std::this_thread::sleep_for(1ms);
    for (const auto& bucket : g_buckets) {
        for (const auto& [key, record] : bucket.entries) rows.emplace_back(key, record);
    }
    return rows;
}

int main() {
    std::atomic<std::uint64_t> operations{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &operations] {
            const int base = t * kKeysPerThread;
            for (int i = 0; i < kKeysPerThread; ++i) insert_key(base + i);
            for (int round = 0; round < kUpdates; ++round) {
                for (int i = 0; i < kKeysPerThread; ++i) update_key(base + i);
            }
            for (int round = 0; round < 3; ++round) {
                for (int i = 0; i < kKeysPerThread; ++i) query_key(base + i);
            }
            for (int i = 0; i < kKeysPerThread; i += 4) erase_key(base + i);
            operations.fetch_add(kKeysPerThread * (1 + kUpdates + 3) + kKeysPerThread / 4,
                                 std::memory_order_relaxed);
        });
    }
    for (auto& thread : threads) thread.join();

    std::size_t total = 0;
    int wrong_value = 0;
    int wrong_mirror = 0;
    for (int key = 0; key < kThreads * kKeysPerThread; ++key) {
        const bool should_exist = (key % kKeysPerThread) % 4 != 0;
        Bucket& bucket = bucket_for(key);
        std::shared_lock<std::shared_mutex> lock(bucket.mutex);
        const auto it = bucket.entries.find(key);
        if (it == bucket.entries.end()) {
            if (should_exist) ++wrong_value;
            continue;
        }
        ++total;
        if (!should_exist) ++wrong_value;
        if (it->second.generation != 1 + kUpdates ||
            it->second.mirrored != it->second.generation + key) {
            ++wrong_value;
        }
        if (it->second.mirrored != it->second.generation + key) ++wrong_mirror;
    }

    std::atomic<bool> holding{false};
    std::atomic<bool> release_snapshot{false};
    std::thread snap([&] {
        const auto rows = snapshot(holding, release_snapshot);
        std::size_t bad = 0;
        for (const auto& [key, record] : rows) {
            if (record.mirrored != record.generation + key) ++bad;
        }
        std::cout << "快照条目 " << rows.size() << "，镜像错误 " << bad << " 处\n";
    });
    while (!holding.load()) std::this_thread::yield();

    std::atomic<bool> writer_started{false};
    std::atomic<bool> writer_done{false};
    std::thread writer([&] {
        writer_started.store(true);
        update_key(0);  // 快照持有全部桶，这个写必须等
        writer_done.store(true);
    });
    while (!writer_started.load()) std::this_thread::yield();
    std::this_thread::sleep_for(30ms);
    const bool blocked = !writer_done.load();
    release_snapshot.store(true);
    snap.join();
    writer.join();

    std::cout << "总操作数 " << operations.load() << '\n';
    std::cout << "校验：条目 " << total << "，值错误 " << wrong_value
              << " 处，镜像错误 " << wrong_mirror << " 处\n";
    std::cout << "快照期间写者被阻塞超过 10 ms：" << (blocked ? "是" : "否") << '\n';
}
