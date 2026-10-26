// 第 21.4 节：用 enable_if 把转发构造函数从 Person 及其派生类、整型上关掉
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Person {
public:
    // 只有当 T 既不是 Person 的派生类、也不是整型时，这个构造函数才存在。
    template <typename T,
              typename = std::enable_if_t<
                  !std::is_base_of_v<Person, std::decay_t<T>> &&
                  !std::is_integral_v<std::remove_reference_t<T>>>>
    explicit Person(T&& name) : name_(std::forward<T>(name)) {
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
    std::string name_;
};

class Employee : public Person {
public:
    explicit Employee(const char* name) : Person(name) {
        std::cout << "Employee 构造\n";
    }

    Employee(const Employee& other) : Person(other) {
        std::cout << "Employee 拷贝构造\n";
    }

    Employee(Employee&& other) noexcept : Person(std::move(other)) {
        std::cout << "Employee 移动构造\n";
    }
};

int main() {
    Person p("Nancy");        // 字符数组：转发构造
    Person q(p);              // 非 const 左值：Person 被排除，走拷贝构造
    const Person cp("Const");
    Person r(cp);             // const 左值：拷贝构造
    Person s(std::move(q));   // 右值：移动构造

    Person t(short{3});       // short 被整型条件排除，走 int 构造
    Person u(3);              // int 构造

    Employee e("Ada");
    Employee e2(e);           // 派生类拷贝：基类部分走 Person 拷贝构造
    Employee e3(std::move(e)); // 派生类移动：基类部分走 Person 移动构造

    std::cout << "p=" << p.name()
              << " r=" << r.name() << " s=" << s.name()
              << " t=" << t.name() << " u=" << u.name() << '\n';

    static_assert(!std::is_base_of_v<Person, std::string>);
    static_assert(std::is_base_of_v<Person, Employee>);
    static_assert(std::is_integral_v<std::remove_reference_t<short&>>);
}
