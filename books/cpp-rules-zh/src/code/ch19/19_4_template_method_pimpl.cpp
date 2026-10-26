// 第 19.4 节：Template Method 固定流程，Pimpl 隐藏实现，扩展协议单独成类
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Record {
    std::string key;
    int value;
};

// 扩展协议：只面向写 worker 的人，不面向 GenericTableAlgorithm 的使用者。
class GTAClient {
public:
    virtual ~GTAClient() = default;
    virtual bool filter(const Record&) const { return true; }
    virtual bool process_row(const Record&) = 0;
};

// 公开接口：只面向调用者。process 是非虚的，流程由实现固定。
class GenericTableAlgorithm {
public:
    GenericTableAlgorithm(std::vector<Record> table, GTAClient& worker);
    ~GenericTableAlgorithm();

    GenericTableAlgorithm(const GenericTableAlgorithm&) = delete;
    GenericTableAlgorithm& operator=(const GenericTableAlgorithm&) = delete;

    bool process();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

struct GenericTableAlgorithm::Impl {
    Impl(std::vector<Record> rows, GTAClient& client)
        : table(std::move(rows)), worker(client) {}

    bool run() {
        std::vector<const Record*> selected;
        for (const auto& record : table) {
            if (worker.filter(record)) {
                selected.push_back(&record);
            }
        }
        for (const Record* record : selected) {
            if (!worker.process_row(*record)) {
                return false;
            }
        }
        return true;
    }

    std::vector<Record> table;
    GTAClient& worker;
};

GenericTableAlgorithm::GenericTableAlgorithm(std::vector<Record> table,
                                             GTAClient& worker)
    : impl_(std::make_unique<Impl>(std::move(table), worker)) {}

// Impl 在类外已经完整，unique_ptr 的删除器才能在这里实例化。
GenericTableAlgorithm::~GenericTableAlgorithm() = default;

bool GenericTableAlgorithm::process() {
    return impl_->run();
}

class SumPositive : public GTAClient {
public:
    bool filter(const Record& record) const override {
        return record.value > 0;
    }

    bool process_row(const Record& record) override {
        total_ += record.value;
        std::cout << "处理 " << record.key << "=" << record.value << '\n';
        return true;
    }

    int total() const { return total_; }

private:
    int total_ = 0;
};

int main() {
    std::vector<Record> table{{"a", 10}, {"b", -5}, {"c", 20}};
    SumPositive worker;
    GenericTableAlgorithm algorithm(std::move(table), worker);

    const bool ok = algorithm.process();
    std::cout << "ok=" << ok << " total=" << worker.total() << '\n';
}
