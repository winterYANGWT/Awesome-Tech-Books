// kvd 的存储层：分桶的哈希表，每个桶一把读写锁。
//
// 一个桶一把锁，而不是整张表一把锁：不同桶的读写可以并行，桶数还是构造参数，
// 第 18 章可以用它演示锁竞争和分片。桶内是普通的 std::unordered_map。
#pragma once

#include "protocol.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class Store {
public:
    explicit Store(std::size_t bucket_count = 64)
        : buckets_(bucket_count == 0 ? 1 : bucket_count) {
        for (auto& bucket : buckets_) bucket = std::make_unique<Bucket>();
    }

    std::optional<std::string> get(std::string_view key) const {
        const Bucket& bucket = bucket_for(key);
        std::shared_lock lock(bucket.mutex);  // 读可以共享
        auto it = bucket.map.find(key);
        if (it == bucket.map.end()) return std::nullopt;
        return it->second;
    }

    void set(std::string_view key, std::string_view value) {
        Bucket& bucket = bucket_for(key);
        std::unique_lock lock(bucket.mutex);  // 写要独占
        bucket.map.insert_or_assign(std::string(key), std::string(value));
    }

    bool del(std::string_view key) {
        Bucket& bucket = bucket_for(key);
        std::unique_lock lock(bucket.mutex);
        auto it = bucket.map.find(key);
        if (it == bucket.map.end()) return false;
        bucket.map.erase(it);
        return true;
    }

    // 只是近似的快照：每个桶单独加锁，统计期间别的线程还能改其他桶。
    // 不一次锁住全部桶，是因为那会把所有写者挡在外面，而 size() 只用来
    // 观察规模；为了一个统计数字牺牲整个服务的吞吐不值得。
    std::size_t size() const {
        std::size_t total = 0;
        for (const auto& bucket : buckets_) {
            std::shared_lock lock(bucket->mutex);
            total += bucket->map.size();
        }
        return total;
    }

    Response apply(const Request& request) {
        switch (request.op) {
        case Request::Op::Get: {
            auto value = get(request.key);
            if (!value) return Response::not_found();
            return Response::value(std::move(*value));
        }
        case Request::Op::Set:
            set(request.key, request.value);
            return Response::ok();
        case Request::Op::Del:
            return del(request.key) ? Response::deleted() : Response::not_found();
        }
        return Response::error("bad op");
    }

private:
    // 让 unordered_map 直接接受 string_view，get/del 不必先在锁里分配一个 string。
    struct StringHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }
    };

    struct Bucket {
        mutable std::shared_mutex mutex;
        std::unordered_map<std::string, std::string, StringHash, std::equal_to<>>
            map;
    };

    Bucket& bucket_for(std::string_view key) {
        return *buckets_[StringHash{}(key) % buckets_.size()];
    }

    const Bucket& bucket_for(std::string_view key) const {
        return *buckets_[StringHash{}(key) % buckets_.size()];
    }

    std::vector<std::unique_ptr<Bucket>> buckets_;
};
