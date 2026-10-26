// 第 23.3 节：关联容器的比较器必须是对象，函数类型没有 operator()
// mode: fail
// expect: function type
#include <set>
#include <string>

using Compare = bool(const std::string&, const std::string&);

int main() {
    // 函数类型可以作模板实参，但 set 要存一个能调用的比较器对象
    std::set<std::string, Compare> names;
    names.insert("ops");
}
