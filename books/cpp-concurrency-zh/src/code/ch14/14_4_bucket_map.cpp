// mode: run
// expect: 条目 3072，值错误 0，快照条目 3072
// 固定 64 个桶，每桶一把 shared_mutex 和一个 list；读用 shared_lock，写用 unique_lock。
#include <atomic>
#include <cstdint>
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
    int value = 0;
};

template <class Key, class Value>
class BucketTable {
    struct Bucket {
        mutable std::shared_mutex mutex;
        std::list<std::pair<Key, Value>> entries;
    };

public:
    explicit BucketTable(std::size_t bucket_count) {
        buckets_.reserve(bucket_count);
        for (std::size_t i = 0; i < bucket_count; ++i) {
            buckets_.push_back(std::make_unique<Bucket>());
        }
    }

    // ANCHOR: api
    void insert_or_assign(const Key& key, Value value) {
        Bucket& bucket = bucket_for(key);
        std::unique_lock<std::shared_mutex> lock(bucket.mutex);
        for (auto& entry : bucket.entries) {
            if (entry.first == key) {
                entry.second = std::move(value);
                return;
            }
        }
        bucket.entries.emplace_back(key, std::move(value));
    }

    bool erase(const Key& key) {
        Bucket& bucket = bucket_for(key);
        std::unique_lock<std::shared_mutex> lock(bucket.mutex);
        for (auto it = bucket.entries.begin(); it != bucket.entries.end(); ++it) {
            if (it->first == key) {
                bucket.entries.erase(it);
                return true;
            }
        }
        return false;
    }

    std::optional<Value> value_for(const Key& key) const {
        const Bucket& bucket = bucket_for(key);
        std::shared_lock<std::shared_mutex> lock(bucket.mutex);
        for (const auto& entry : bucket.entries) {
            if (entry.first == key) {
                return entry.second;
            }
        }
        return std::nullopt;
    }
    // ANCHOR_END: api

    // ANCHOR: snapshot
    // 一致快照：按桶下标递增的顺序锁住全部桶，复制完再逆序解锁。
    std::map<Key, Value> snapshot() const {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        locks.reserve(buckets_.size());
        for (const auto& bucket : buckets_) {
            locks.emplace_back(bucket->mutex);
        }

        std::map<Key, Value> copy;
        for (const auto& bucket : buckets_) {
            for (const auto& entry : bucket->entries) {
                copy.insert(entry);
            }
        }

        for (auto it = locks.rbegin(); it != locks.rend(); ++it) {
            it->unlock();
        }
        return copy;
    }
    // ANCHOR_END: snapshot

private:
    Bucket& bucket_for(const Key& key) {
        const std::size_t index = std::hash<Key>{}(key) % buckets_.size();
        return *buckets_[index];
    }

    const Bucket& bucket_for(const Key& key) const {
        const std::size_t index = std::hash<Key>{}(key) % buckets_.size();
        return *buckets_[index];
    }

    std::vector<std::unique_ptr<Bucket>> buckets_;
};

constexpr int kBuckets = 64;
constexpr int kThreads = 8;
constexpr int kKeysPerThread = 512;
constexpr int kUpdates = 3;

}  // namespace

int main() {
    BucketTable<int, Record> table(kBuckets);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            const int base = t * kKeysPerThread;
            for (int i = 0; i < kKeysPerThread; ++i) {
                const int key = base + i;
                table.insert_or_assign(key, Record{1, 1000 + key});
            }
            for (int round = 0; round < kUpdates; ++round) {
                for (int i = 0; i < kKeysPerThread; ++i) {
                    const int key = base + i;
                    auto record = table.value_for(key);
                    if (record) {
                        ++record->generation;
                        record->value = record->generation * 1000 + key;
                        table.insert_or_assign(key, *record);
                    }
                }
            }
            for (int round = 0; round < 2; ++round) {
                for (int i = 0; i < kKeysPerThread; ++i) {
                    (void)table.value_for(base + i);
                }
            }
            for (int i = 0; i < kKeysPerThread; i += 4) {
                table.erase(base + i);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    int total = 0;
    int wrong = 0;
    for (int key = 0; key < kThreads * kKeysPerThread; ++key) {
        const bool should_exist = key % 4 != 0;
        const auto record = table.value_for(key);
        if (record.has_value() != should_exist) {
            ++wrong;
            continue;
        }
        if (!record) {
            continue;
        }
        ++total;
        if (record->generation != 1 + kUpdates ||
            record->value != record->generation * 1000 + key) {
            ++wrong;
        }
    }

    const auto copy = table.snapshot();
    std::printf("条目 %d，值错误 %d，快照条目 %zu\n", total, wrong, copy.size());
    return 0;
}
