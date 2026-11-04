// mode: run
// expect: 快照 8 次，条目 2048，半更新 0，重复 0
// 写者只改一个桶；快照按 0..63 的顺序锁住全部桶，因此不会把两个时刻拼在一起。
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct Record {
    int generation = 0;
    int mirrored = 0;  // 必须等于 generation + key
};

class Table {
    struct Bucket {
        mutable std::shared_mutex mutex;
        std::list<std::pair<int, Record>> entries;
    };

public:
    explicit Table(std::size_t bucket_count) {
        buckets_.reserve(bucket_count);
        for (std::size_t i = 0; i < bucket_count; ++i) {
            buckets_.push_back(std::make_unique<Bucket>());
        }
    }

    void insert(int key, Record record) {
        Bucket& bucket = bucket_for(key);
        std::unique_lock<std::shared_mutex> lock(bucket.mutex);
        bucket.entries.emplace_back(key, record);
    }

    void update(int key) {
        Bucket& bucket = bucket_for(key);
        std::unique_lock<std::shared_mutex> lock(bucket.mutex);
        for (auto& entry : bucket.entries) {
            if (entry.first == key) {
                ++entry.second.generation;
                entry.second.mirrored = entry.second.generation + key;
                return;
            }
        }
    }

    // ANCHOR: snapshot
    std::map<int, Record> snapshot() const {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        locks.reserve(buckets_.size());
        for (const auto& bucket : buckets_) {
            locks.emplace_back(bucket->mutex);  // 必须按下标递增顺序
        }

        std::map<int, Record> copy;
        for (const auto& bucket : buckets_) {
            for (const auto& entry : bucket->entries) {
                copy.emplace(entry.first, entry.second);
            }
        }

        for (auto it = locks.rbegin(); it != locks.rend(); ++it) {
            it->unlock();
        }
        return copy;
    }
    // ANCHOR_END: snapshot

private:
    Bucket& bucket_for(int key) {
        return *buckets_[std::hash<int>{}(key) % buckets_.size()];
    }

    const Bucket& bucket_for(int key) const {
        return *buckets_[std::hash<int>{}(key) % buckets_.size()];
    }

    std::vector<std::unique_ptr<Bucket>> buckets_;
};

constexpr int kBuckets = 64;
constexpr int kKeys = 2048;
constexpr int kWriters = 4;

}  // namespace

int main() {
    Table table(kBuckets);
    for (int key = 0; key < kKeys; ++key) {
        table.insert(key, Record{0, key});
    }

    std::atomic<bool> stop{false};
    std::vector<std::thread> writers;
    for (int w = 0; w < kWriters; ++w) {
        writers.emplace_back([&, w] {
            int key = w;
            while (!stop.load(std::memory_order_relaxed)) {
                table.update(key);
                key += kWriters;
                if (key >= kKeys) {
                    key = w;
                }
            }
        });
    }

    int half_updated = 0;
    int duplicated = 0;
    std::size_t entries = 0;
    constexpr int kRounds = 8;
    for (int round = 0; round < kRounds; ++round) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const auto copy = table.snapshot();
        entries = copy.size();
        for (const auto& [key, record] : copy) {
            if (record.mirrored != record.generation + key) {
                ++half_updated;
            }
        }
        duplicated += kKeys - static_cast<int>(copy.size());
    }

    stop.store(true);
    for (auto& thread : writers) {
        thread.join();
    }

    std::printf("快照 %d 次，条目 %zu，半更新 %d，重复 %d\n", kRounds, entries, half_updated,
                duplicated);
    return 0;
}
