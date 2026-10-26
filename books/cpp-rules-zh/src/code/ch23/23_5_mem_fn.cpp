// 第 23.5 节：mem_fn、not_fn、invoke 取代 mem_fun、not1 和 ptr_fun
#include <algorithm>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class Employee {
public:
    Employee(std::string name, int salary) : name_(std::move(name)), salary_(salary) {}

    void raise() { salary_ += 1000; }
    bool highlyPaid() const { return salary_ > 5000; }
    const std::string& name() const { return name_; }
    int salary() const { return salary_; }

private:
    std::string name_;
    int salary_;
};

// 普通函数：not_fn 直接用，不需要 ptr_fun 补 typedef
bool shortName(const Employee& employee) {
    return employee.name().size() <= 3;
}

// 检测某个类型有没有 C++98 适配器要的 argument_type
template <typename T, typename = void>
struct has_argument_type : std::false_type {};
template <typename T>
struct has_argument_type<T, std::void_t<typename T::argument_type>> : std::true_type {};

int main() {
    std::vector<Employee> staff{{"Ann", 4000}, {"Bartholomew", 5000}};
    std::vector<std::unique_ptr<Employee>> contractors;
    contractors.push_back(std::make_unique<Employee>("Cy", 6000));

    // 同一个 mem_fn 对象，对象、裸指针、unique_ptr 都能接
    auto raise = std::mem_fn(&Employee::raise);
    raise(staff[0]);
    raise(contractors[0]);
    raise(contractors[0].get());

    // bind_front 也能直接绑定成员函数指针
    auto raiseBy = std::bind_front(&Employee::raise);
    raiseBy(staff[1]);

    std::printf("Ann = %d，Bartholomew = %d，Cy = %d\n",
                std::invoke(&Employee::salary, staff[0]),
                std::invoke(&Employee::salary, staff[1]),
                std::invoke(&Employee::salary, contractors[0]));

    auto notShort = std::not_fn(shortName);
    // 这些新适配器不再要求 argument_type / result_type
    static_assert(!has_argument_type<decltype(notShort)>::value);

    auto notHighlyPaid = std::not_fn(&Employee::highlyPaid);
    std::printf("名字不短的人数：%zu\n",
                static_cast<std::size_t>(std::count_if(staff.begin(), staff.end(), notShort)));
    std::printf("工资不高于 5000 的人数：%zu\n",
                static_cast<std::size_t>(std::count_if(staff.begin(), staff.end(), notHighlyPaid)));

    // C++20 的 ranges 算法按 std::invoke 的规则调用，成员函数指针可以直接传
    std::vector<Employee> newcomer{{"Dee", 1000}};
    std::ranges::for_each(newcomer, &Employee::raise);
    std::printf("ranges::for_each 直接收成员指针：Dee = %d\n",
                std::invoke(&Employee::salary, newcomer[0]));
}
