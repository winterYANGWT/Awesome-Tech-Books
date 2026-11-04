// mode: run
// expect: 总归并次数：7，层数：3
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <thread>
#include <vector>

using Seq = std::vector<int>;

Seq merge_two(const Seq& a, const Seq& b) {
    Seq out;
    out.reserve(a.size() + b.size());
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
    return out;
}

int main() {
    constexpr int kSegments = 8;
    constexpr int kPerSegment = 1000;

    // 8 段数据先各自排序，作为归并的起点。
    std::mt19937 rng(42);
    std::vector<Seq> level(kSegments, Seq(kPerSegment));
    for (auto& seg : level) {
        for (int& v : seg) v = static_cast<int>(rng() % 100000);
        std::sort(seg.begin(), seg.end());
    }

    int total_merges = 0;
    int layers = 0;
    while (level.size() > 1) {
        // 同一层的每一对互不依赖，可以各开一个线程同时归并。
        std::vector<Seq> next(level.size() / 2);
        std::vector<std::thread> workers;
        for (std::size_t i = 0; i < next.size(); ++i) {
            workers.emplace_back([&, i] {
                next[i] = merge_two(level[2 * i], level[2 * i + 1]);
            });
        }
        for (auto& t : workers) t.join();

        total_merges += static_cast<int>(next.size());
        ++layers;
        std::cout << "第 " << layers << " 层：" << next.size()
                  << " 次归并，" << next.size() << " 个结果段\n";
        level = std::move(next);
    }

    std::cout << "总归并次数：" << total_merges << "，层数：" << layers << "\n";
    std::cout << "最终长度：" << level[0].size()
              << "，已排序：" << std::is_sorted(level[0].begin(), level[0].end())
              << "\n";
}
