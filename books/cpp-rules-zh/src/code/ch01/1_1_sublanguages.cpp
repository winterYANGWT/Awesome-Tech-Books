// 第 1.1 节：同一条“按值传递”规则，在 C、面向对象和 STL 里代价不同

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Record {
public:
    explicit Record(std::string name) : name_(std::move(name)) {}
    Record(const Record& other) : name_(other.name_) { ++copies; }
    Record(Record&& other) noexcept : name_(std::move(other.name_)) { ++moves; }
    Record& operator=(const Record&) = default;
    Record& operator=(Record&&) = default;
    ~Record() = default;

    const std::string& name() const { return name_; }

    static void reset() { copies = 0; moves = 0; }

    static inline int copies = 0;
    static inline int moves = 0;

private:
    std::string name_;
};

// 面向对象子语言的习惯：只读参数用 const&，不复制对象。
int by_const_ref(const Record& record) {
    return static_cast<int>(record.name().size());
}

// 同一个函数改成按值传参，就会调用一次拷贝构造。
int by_value(Record record) {
    return static_cast<int>(record.name().size());
}

// C 子语言的习惯：内置类型按值传，寄存器里就完成了。
long add_c(long a, long b) {
    return a + b;
}

// STL 的函数对象按值传递；想观察调用次数，得用 std::ref 把状态留在外面。
struct CountCalls {
    int calls = 0;

    void operator()(const Record&) { ++calls; }
};

int main() {
    Record record("order-2024");

    std::cout << "C long by value: " << add_c(20, 22)
              << ", Record copies=" << Record::copies << '\n';

    Record::reset();
    by_value(record);
    std::cout << "OO by value: copies=" << Record::copies
              << " moves=" << Record::moves << '\n';

    Record::reset();
    by_const_ref(record);
    std::cout << "OO by const&: copies=" << Record::copies
              << " moves=" << Record::moves << '\n';

    Record::reset();
    by_value(std::move(record));
    std::cout << "OO by value from rvalue: copies=" << Record::copies
              << " moves=" << Record::moves << '\n';

    std::vector<Record> records;
    records.emplace_back("a");
    records.emplace_back("bb");
    records.emplace_back("ccc");

    CountCalls returned = std::for_each(records.begin(), records.end(), CountCalls{});
    std::cout << "STL returned functor calls=" << returned.calls << '\n';

    CountCalls observed;
    std::for_each(records.begin(), records.end(), std::ref(observed));
    std::cout << "STL std::ref calls=" << observed.calls << '\n';
}
