// 第 29.1 节：避免本来就更容易写的低效写法，并把这些成本数出来
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Report {
    std::string customer;
    double total = 0.0;

    static inline std::size_t copies = 0;
    static inline std::size_t moves = 0;

    Report(std::string name, double amount)
        : customer(std::move(name)), total(amount) {}

    Report(const Report& other)
        : customer(other.customer), total(other.total) {
        ++copies;
    }

    Report(Report&& other) noexcept
        : customer(std::move(other.customer)), total(other.total) {
        ++moves;
    }

    Report& operator=(const Report&) = default;
    Report& operator=(Report&&) noexcept = default;
    ~Report() = default;
};

// 按值接收：整个 vector 连同每个 Report 都要复制一份。
double summarize_by_value(std::vector<Report> reports) {
    double sum = 0.0;
    for (const Report& r : reports) sum += r.total;
    return sum;
}

// 只读按 const 引用：不复制任何元素。
double summarize_by_ref(const std::vector<Report>& reports) {
    double sum = 0.0;
    for (const Report& r : reports) sum += r.total;
    return sum;
}

int main() {
    std::vector<Report> reports;
    reports.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        reports.emplace_back("customer-" + std::to_string(i), i * 1.5);
    }

    Report::copies = 0;
    Report::moves = 0;
    const double by_value = summarize_by_value(reports);
    std::cout << "按值传 vector:  copies=" << Report::copies
              << "  moves=" << Report::moves
              << "  sum=" << by_value << '\n';

    Report::copies = 0;
    Report::moves = 0;
    const double by_ref = summarize_by_ref(reports);
    std::cout << "按 const 引用:  copies=" << Report::copies
              << "  moves=" << Report::moves
              << "  sum=" << by_ref << '\n';

    // 不 reserve 时 vector 反复扩容，已构造的元素要被搬走。
    auto build = [](bool reserve_first) {
        std::vector<Report> v;
        if (reserve_first) v.reserve(1000);
        Report::copies = 0;
        Report::moves = 0;
        std::size_t reallocations = 0;
        std::size_t capacity = v.capacity();
        for (int i = 0; i < 1000; ++i) {
            v.emplace_back("customer-" + std::to_string(i), i * 1.5);
            if (v.capacity() != capacity) {
                capacity = v.capacity();
                ++reallocations;
            }
        }
        std::cout << (reserve_first ? "reserve(1000): " : "不 reserve:    ")
                  << "reallocations=" << reallocations
                  << "  moves=" << Report::moves
                  << "  capacity=" << v.capacity() << '\n';
    };
    build(false);
    build(true);

    // s = s + "ab" 每次都构造新字符串并重抄旧内容；s += "ab" 只在扩容时搬一次。
    std::size_t full_copies_plus = 0;
    std::size_t full_copies_append = 0;
    {
        std::string s;
        for (int i = 0; i < 1000; ++i) {
            s = s + "ab";               // 新缓冲区 + 复制整个旧值
            ++full_copies_plus;
        }
    }
    {
        std::string s;
        std::size_t capacity = s.capacity();
        for (int i = 0; i < 1000; ++i) {
            s += "ab";
            if (s.capacity() != capacity) {
                capacity = s.capacity();
                ++full_copies_append;   // 只有扩容时才整体搬一次
            }
        }
    }
    std::cout << "s = s + \"ab\" 整体复制次数: " << full_copies_plus << '\n';
    std::cout << "s += \"ab\" 整体复制次数:    " << full_copies_append << '\n';
}
