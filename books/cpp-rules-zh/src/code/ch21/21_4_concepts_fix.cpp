// 第 21.4 节：把“不是 Person 派生类、不是整型、能构造 string”写成一句 concept
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Person;

template <typename T>
concept PersonSource =
    !std::derived_from<std::remove_cvref_t<T>, Person> &&
    !std::integral<std::remove_cvref_t<T>> &&
    std::constructible_from<std::string, T>;

class Person {
public:
    // 约束在模板头部，接口直接说明“什么样实参能构造 Person”。
    template <PersonSource T>
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
    Person q(p);              // 非 const 左值：被 concept 排除，走拷贝构造
    const Person cp("Const");
    Person r(cp);             // const 左值：拷贝构造
    Person s(std::move(q));   // 右值：移动构造

    Person t(short{3});       // short 是整型，被排除，走 int 构造
    Person u(3);              // int 构造

    Employee e("Ada");
    Employee e2(e);           // 派生类拷贝：基类部分走 Person 拷贝构造
    Employee e3(std::move(e)); // 派生类移动：基类部分走 Person 移动构造

    std::cout << "p=" << p.name()
              << " r=" << r.name() << " s=" << s.name()
              << " t=" << t.name() << " u=" << u.name() << '\n';

    static_assert(PersonSource<const char*>);
    static_assert(PersonSource<const char (&)[6]>);
    static_assert(PersonSource<std::string>);
    static_assert(!PersonSource<Person&>);
    static_assert(!PersonSource<Person&&>);
    static_assert(!PersonSource<const Person&>);
    static_assert(!PersonSource<int>);
    static_assert(!PersonSource<short>);
}
