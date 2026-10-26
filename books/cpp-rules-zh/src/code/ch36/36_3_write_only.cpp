// 第 36.3 节：同一段逻辑，一行式写法与拆成具名步骤的写法结果相同，阅读成本不同
// mode: run
#include <algorithm>
#include <iostream>
#include <vector>

struct LogEntry {
    int id;
    int level;
    int weight;
};

std::vector<LogEntry> keep_after_checkpoint_dense(std::vector<LogEntry> entries) {
    // 从最后一次 level >= 3 的记录之后开始，删掉 level < 2 的记录
    entries.erase(
        std::remove_if(
            std::find_if(entries.rbegin(), entries.rend(),
                         [](const LogEntry& entry) { return entry.level >= 3; })
                .base(),
            entries.end(), [](const LogEntry& entry) { return entry.level < 2; }),
        entries.end());
    return entries;
}

bool is_checkpoint(const LogEntry& entry) { return entry.level >= 3; }
bool is_noise(const LogEntry& entry) { return entry.level < 2; }

std::vector<LogEntry> keep_after_checkpoint(std::vector<LogEntry> entries) {
    const auto checkpoint = std::find_if(entries.rbegin(), entries.rend(), is_checkpoint);
    const auto first_after =
        checkpoint == entries.rend() ? entries.begin() : checkpoint.base();
    // 反向迭代器的 base() 指向原序列中该元素的下一个位置：这里正好是 checkpoint 之后的第一条
    const auto noise = std::remove_if(first_after, entries.end(), is_noise);
    entries.erase(noise, entries.end());
    return entries;
}

int bonus_points_dense(const std::vector<LogEntry>& entries) {
    int total = 0;
    for (const auto& entry : entries) {
        if (entry.level >= 2) {
            if (entry.weight > 0) {
                for (int i = 0; i < entry.weight; ++i) {
                    if (i % 3 != 0) {
                        if (entry.id % 2 == 0) {
                            total += 2;
                        } else {
                            total += 1;
                        }
                    }
                }
            }
        }
    }
    return total;
}

bool contributes_bonus(const LogEntry& entry) {
    return entry.level >= 2 && entry.weight > 0;
}

int points_per_unit(const LogEntry& entry) { return entry.id % 2 == 0 ? 2 : 1; }

int scored_units(int weight) {
    int units = 0;
    for (int i = 0; i < weight; ++i) {
        if (i % 3 != 0) {
            ++units;
        }
    }
    return units;
}

int bonus_points(const std::vector<LogEntry>& entries) {
    int total = 0;
    for (const auto& entry : entries) {
        if (!contributes_bonus(entry)) {
            continue;  // 不满足前提就跳过，循环体保持一层
        }
        total += points_per_unit(entry) * scored_units(entry.weight);
    }
    return total;
}

void print_ids(const char* label, const std::vector<LogEntry>& entries) {
    std::cout << label;
    for (const auto& entry : entries) {
        std::cout << ' ' << entry.id;
    }
    std::cout << '\n';
}

int main() {
    const std::vector<LogEntry> log{
        {1, 1, 2}, {2, 3, 4}, {3, 1, 1}, {4, 2, 6}, {5, 3, 2}, {6, 0, 3}, {7, 1, 5},
    };

    const auto dense = keep_after_checkpoint_dense(log);
    const auto clear = keep_after_checkpoint(log);
    print_ids("dense:", dense);
    print_ids("clear:", clear);
    const bool same = std::equal(dense.begin(), dense.end(), clear.begin(), clear.end(),
                                 [](const LogEntry& a, const LogEntry& b) { return a.id == b.id; });
    std::cout << "same entries: " << (same ? "yes" : "no") << '\n';

    std::cout << "bonus dense: " << bonus_points_dense(log) << '\n';
    std::cout << "bonus clear: " << bonus_points(log) << '\n';
    if (!same || bonus_points_dense(log) != bonus_points(log)) {
        return 1;
    }
}
