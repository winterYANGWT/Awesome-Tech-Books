// 第 21.4 节：通用引用重载对几乎任何实参都是精确匹配，会抢走普通重载
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

void record(int idx) {
    std::cout << "int 重载: " << idx << '\n';
}

void record(const std::string& name) {
    std::cout << "const string& 重载: " << name << '\n';
}

template <typename T>
void record(T&& value) {
    std::cout << "转发重载: " << value << '\n';
}

class Person {
public:
    // 真正的转发构造会用 value 初始化 name_；这里只记录被选中的构造，
    // 以便观察重载决议；按真实写法会编译失败的版本在下一例。
    template <typename T>
    explicit Person(T&&) : name_("forwarded") {
        std::cout << "Person 转发构造\n";
    }

    explicit Person(int idx) : name_("idx-" + std::to_string(idx)) {
        std::cout << "Person int 构造\n";
    }

    Person(const Person& other) : name_(other.name_) {
        std::cout << "Person 拷贝构造\n";
    }

    Person(Person&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "Person 移动构造\n";
    }

    const std::string& name() const { return name_; }

private:
    std::string name_ = "unnamed";
};

class Employee : public Person {
public:
    explicit Employee(const char* name) : Person(name) {}

    Employee(const Employee& other) : Person(other) {
        std::cout << "Employee 拷贝构造\n";
    }

    Employee(Employee&& other) noexcept : Person(std::move(other)) {
        std::cout << "Employee 移动构造\n";
    }
};

int main() {
    std::cout << "自由函数：\n";
    record(7);              // int 实参，非模板的 int 重载精确匹配
    record(short{7});       // short 提升到 int 才能调用 int 重载，转发重载更匹配
    record("Patty");        // 转发重载接受字符数组
    std::string draft("draft");
    const std::string invoice("invoice");
    record(draft);          // 非 const 左值：T&& 比 const string& 更匹配
    record(invoice);        // const 左值：非模板的 const string& 胜出

    std::cout << "Person 构造：\n";
    Person p("Nancy");      // 转发构造
    Person q(p);            // 非 const 左值：转发构造比拷贝构造更匹配
    const Person cp("Const");
    Person r(cp);           // const 左值：拷贝构造精确匹配，非模板优先
    Person s(std::move(q)); // 右值：移动构造是非模板，优先于转发构造
    Person t(short{3});     // short：转发构造胜过需要提升的 int 构造
    Person u(3);            // int：非模板的 int 构造胜出

    std::cout << "派生类：\n";
    Employee e("Ada");
    Employee e2(e);              // 基类部分走 Person 的转发构造，而不是拷贝构造
    Employee e3(std::move(e));   // 移动时同样走转发构造

    std::cout << "p=" << p.name() << " r=" << r.name()
              << " s=" << s.name() << " t=" << t.name()
              << " u=" << u.name() << '\n';

    static_assert(std::is_integral_v<int>);
    static_assert(!std::is_integral_v<int&>);  // 引用不是整型，T 可能是 int&
    static_assert(std::is_integral_v<std::remove_reference_t<int&>>);
}
