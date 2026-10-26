// 第 25.4 节：新增键与更新已有键走不同路径；数一数构造和赋值
// mode: run
#include <iostream>
#include <map>
#include <string>
#include <utility>

struct Price {
    double amount = 0.0;
    static inline int default_ctor = 0;
    static inline int value_ctor = 0;
    static inline int assign = 0;

    Price() {
        ++default_ctor;
    }
    explicit Price(double value) : amount(value) {
        ++value_ctor;
    }
    Price& operator=(double value) {
        amount = value;
        ++assign;
        return *this;
    }
};

static void report(const char* what) {
    std::cout << what << "：默认构造 " << Price::default_ctor
              << "，值构造 " << Price::value_ctor
              << "，赋值 " << Price::assign << '\n';
    Price::default_ctor = 0;
    Price::value_ctor = 0;
    Price::assign = 0;
}

int main() {
    std::map<int, Price> catalog;

    catalog[1] = 12.5;  // 新键：先默认构造 value，再赋值
    report("catalog[1] = 12.5（新键）");

    catalog[2] = 20.0;
    Price::default_ctor = 0;
    Price::value_ctor = 0;
    Price::assign = 0;
    catalog[2] = 25.0;  // 已有键：只赋值，不构造 value
    report("catalog[2] = 25.0（已有键）");

    catalog.emplace(3, Price{30.0});  // 先造 Price，再移动进节点
    report("emplace(3, Price{30.0})（新键）");

    catalog.try_emplace(4, 40.0);  // 直接用 double 在节点里构造
    report("try_emplace(4, 40.0)（新键）");

    catalog.try_emplace(1, 99.0);  // 键已存在：实参根本不构造
    report("try_emplace(1, 99.0)（已有键）");

    catalog.insert_or_assign(1, 55.0);  // 已有键：赋值
    report("insert_or_assign(1, 55.0)（已有键）");

    catalog.insert_or_assign(5, 77.0);  // 新键：就地构造
    report("insert_or_assign(5, 77.0)（新键）");

    catalog.insert({1, Price{88.0}});  // 已有键：整个 pair 白造，值不变
    report("insert({1, Price{88.0}})（已有键）");
    std::cout << "catalog.at(1).amount 仍然是 " << catalog.at(1).amount << '\n';
}
