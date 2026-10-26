// 第 21.4 节：完美转发构造函数抢走拷贝构造，随后在 std::string 构造处失败
// mode: fail
// expect: no matching constructor
#include <string>
#include <utility>

class Person {
public:
    template <typename T>
    explicit Person(T&& name) : name_(std::forward<T>(name)) {}

private:
    std::string name_;
};

int main() {
    Person p("Nancy");
    Person q(p);   // 非 const 左值：模板比拷贝构造更匹配
}
