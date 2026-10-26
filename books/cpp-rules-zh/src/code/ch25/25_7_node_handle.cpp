// 第 25.3 节：C++17 节点句柄：取出节点、改键或改非键部分、再插回
// mode: run
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>

struct Employee {
    int id;
    std::string name;
    std::string title;
};

struct ById {
    bool operator()(const Employee& a, const Employee& b) const {
        return a.id < b.id;
    }
};

int main() {
    std::map<int, std::string> badges{{13, "Alice"}, {7, "Bob"}};

    // 改 map 的键：extract 取出节点，key() 返回可写的 Key&
    auto node = badges.extract(13);
    node.key() = 42;
    auto result = badges.insert(std::move(node));
    std::cout << std::boolalpha;
    std::cout << "改键后 inserted=" << result.inserted
              << "，新键=" << result.position->first
              << "，值=" << result.position->second << '\n';

    // 新键已被占用：insert 失败，节点句柄仍持有元素，可以改回原键再插
    auto clash = badges.extract(42);
    clash.key() = 7;
    auto failed = badges.insert(std::move(clash));
    std::cout << "撞键后 inserted=" << failed.inserted
              << "，句柄里的键=" << failed.node.key() << '\n';
    failed.node.key() = 42;                 // 改回原来的键
    badges.insert(std::move(failed.node));  // 没有这句，节点析构时元素就丢了
    std::cout << "放回后 map 内容：";
    for (const auto& [id, name] : badges) {
        std::cout << id << "=" << name << ' ';
    }
    std::cout << '\n';

    // 改 set 元素的非键部分：value() 返回 T&
    std::set<Employee, ById> staff{{1, "Carol", "Engineer"}, {2, "Dave", "Analyst"}};
    auto employee = staff.extract(Employee{1, "", ""});
    employee.value().title = "Manager";
    staff.insert(std::move(employee));
    std::cout << "改 title 后：";
    for (const auto& e : staff) {
        std::cout << e.id << ":" << e.name << ":" << e.title << ' ';
    }
    std::cout << '\n';

    // C++17 之前的可移植写法：拷贝、erase、改副本、用后继位置作 hint 插回
    auto it = staff.find(Employee{2, "", ""});
    if (it != staff.end()) {
        Employee copy = *it;
        staff.erase(it++);       // it 前进到后继，erase 吃掉旧位置的元素
        copy.title = "Lead";
        staff.insert(it, copy);  // hint 指向原位置的后继
    }
    std::cout << "erase + insert 后：";
    for (const auto& e : staff) {
        std::cout << e.id << ":" << e.title << ' ';
    }
    std::cout << '\n';
}
