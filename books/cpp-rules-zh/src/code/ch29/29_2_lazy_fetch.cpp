// 第 29.2 节：惰性取数，只把真正访问过的字段读出来
#include <iostream>
#include <optional>
#include <string>

// 假装是一个远程数据库：每取一个字段就算一次查询。
namespace db {
inline int queries = 0;

std::string field(int id, const std::string& name) {
    ++queries;
    return name + "-" + std::to_string(id);
}
}  // namespace db

// 饿汉式：构造时把四个字段全部取回。
class CustomerEager {
public:
    explicit CustomerEager(int id)
        : name_(db::field(id, "name")),
          city_(db::field(id, "city")),
          plan_(db::field(id, "plan")) {}

    const std::string& name() const { return name_; }
    const std::string& city() const { return city_; }
    const std::string& plan() const { return plan_; }

private:
    std::string name_;
    std::string city_;
    std::string plan_;
};

// 懒汉式：构造只记 id；字段第一次被访问时才查询，并把结果缓存起来。
class CustomerLazy {
public:
    explicit CustomerLazy(int id) : id_(id) {}

    const std::string& name() const {
        if (!name_) name_ = db::field(id_, "name");
        return *name_;
    }

    const std::string& city() const {
        if (!city_) city_ = db::field(id_, "city");
        return *city_;
    }

    const std::string& plan() const {
        if (!plan_) plan_ = db::field(id_, "plan");
        return *plan_;
    }

private:
    int id_;
    // const 成员函数也要能填缓存，所以缓存指针/optional 声明为 mutable。
    mutable std::optional<std::string> name_;
    mutable std::optional<std::string> city_;
    mutable std::optional<std::string> plan_;
};

int main() {
    constexpr int customers = 100;

    // 场景一：只关心其中 10 个客户的 city，其他字段根本不需要。
    db::queries = 0;
    for (int i = 0; i < customers; ++i) {
        CustomerEager c(i);
        if (i < 10) std::cout << "eager city: " << c.city() << '\n';
    }
    std::cout << "eager 只读 10 个 city，字段查询次数: " << db::queries << '\n';

    db::queries = 0;
    for (int i = 0; i < customers; ++i) {
        CustomerLazy c(i);
        if (i < 10) std::cout << "lazy  city: " << c.city() << '\n';
    }
    std::cout << "lazy  只读 10 个 city，字段查询次数: " << db::queries << '\n';

    // 场景二：每个客户的每个字段都读。惰性没有省下查询，还多了一次分支判断。
    db::queries = 0;
    for (int i = 0; i < customers; ++i) {
        CustomerEager c(i);
        (void)c.name();
        (void)c.city();
        (void)c.plan();
    }
    std::cout << "eager 全读，字段查询次数: " << db::queries << '\n';

    db::queries = 0;
    for (int i = 0; i < customers; ++i) {
        CustomerLazy c(i);
        (void)c.name();
        (void)c.city();
        (void)c.plan();
    }
    std::cout << "lazy  全读，字段查询次数: " << db::queries << '\n';
}
