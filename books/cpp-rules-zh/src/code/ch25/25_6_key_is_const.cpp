// 第 25.3 节：map 的键是 const，set 的迭代器是常量迭代器，原地改键都不合法
// mode: fail
// expect: error
#include <map>
#include <set>
#include <string>

struct Employee {
    int id;
    std::string title;
};

struct ById {
    bool operator()(const Employee& a, const Employee& b) const {
        return a.id < b.id;
    }
};

int main() {
    std::map<int, std::string> names{{13, "Alice"}};
    names.begin()->first = 9999999;  // 错：元素类型是 pair<const int, string>

    std::set<Employee, ById> staff{{1, "Engineer"}};
    staff.begin()->title = "Manager";  // 错：set 的 iterator 解引用得到 const Employee&
}
