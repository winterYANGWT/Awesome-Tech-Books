// mode: run
// 固定容量的 DNS 缓存：查询走 shared_lock，更新走 unique_lock。
#include <array>
#include <iostream>
#include <latch>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

class DnsCache {
public:
    static constexpr std::size_t kCapacity = 128;

    bool find(const std::string& domain, std::string& address) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);  // 多个查询可以同时进来
        const auto it = entries_.find(domain);
        if (it == entries_.end()) return false;
        address = it->second;
        return true;
    }

    bool update(const std::string& domain, const std::string& address) {
        std::unique_lock<std::shared_mutex> lock(mutex_);  // 更新独占
        const auto it = entries_.find(domain);
        if (it != entries_.end()) {
            it->second = address;
            return true;
        }
        if (entries_.size() >= kCapacity) return false;
        entries_.emplace(domain, address);
        return true;
    }

    std::size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return entries_.size();
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::string> entries_;
};

int main() {
    constexpr int kReaders = 8;
    constexpr int kQueriesPerReader = 50'000;
    constexpr int kUpdates = 20'000;

    DnsCache cache;
    for (int i = 0; i < 64; ++i) {
        cache.update("site" + std::to_string(i), "10.0.0." + std::to_string(i));
    }

    std::latch start(kReaders + 2);  // 8 个读者 + 1 个写者 + main
    std::array<long, kReaders> hits{};
    std::vector<std::thread> threads;

    for (int t = 0; t < kReaders; ++t) {
        threads.emplace_back([&, t] {
            start.arrive_and_wait();
            long local_hits = 0;
            for (int i = 0; i < kQueriesPerReader; ++i) {
                std::string address;
                const int key = i % 64;
                if (cache.find("site" + std::to_string(key), address)) ++local_hits;
            }
            hits[t] = local_hits;
        });
    }
    threads.emplace_back([&] {
        start.arrive_and_wait();
        for (int i = 0; i < kUpdates; ++i) {
            if (i < 64) {
                cache.update("site" + std::to_string(i), "10.1.0." + std::to_string(i));
            } else if (i < 128) {
                cache.update("new" + std::to_string(i - 64), "10.2.0." + std::to_string(i - 64));
            } else {
                cache.update("site" + std::to_string(i % 64), "10.3.0." + std::to_string(i));
            }
        }
    });

    start.arrive_and_wait();
    for (auto& thread : threads) thread.join();

    long total_hits = 0;
    for (long h : hits) total_hits += h;
    const long total_queries = long{kReaders} * kQueriesPerReader;
    std::cout << kReaders << " 个读线程 × " << kQueriesPerReader
              << " 次查询，1 个写线程 × " << kUpdates << " 次更新\n";
    std::cout << "命中 " << total_hits << " 次，未命中 " << total_queries - total_hits << " 次\n";
    std::cout << "最终条目 " << cache.size() << " / 容量 " << DnsCache::kCapacity << '\n';
}
