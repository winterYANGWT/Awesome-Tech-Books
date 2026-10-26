// 第 18.5 节：纯虚函数可以有定义，纯虚析构必须有定义
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

class Task {
public:
    virtual ~Task() = 0;  // 纯虚析构：让 Task 抽象，又允许多态删除

    virtual std::string run() const = 0;

    // 纯虚函数也可以有函数体；派生类必须覆盖，想用默认实现要显式写 Task::describe()。
    virtual std::string describe() const = 0;
};

Task::~Task() = default;  // 派生类析构一定会调用它，所以必须有定义

std::string Task::describe() const { return "task"; }

class RetryTask : public Task {
public:
    std::string run() const override { return "retry"; }
    std::string describe() const override { return "RetryTask(" + Task::describe() + ")"; }
};

class SilentTask : public Task {
public:
    std::string run() const override { return "silent"; }
    std::string describe() const override { return Task::describe(); }  // 显式选择默认实现
};

// 没有覆盖 describe，仍然是抽象类。
class StillAbstract : public Task {
public:
    std::string run() const override { return "never"; }
};

int main() {
    std::unique_ptr<Task> task = std::make_unique<RetryTask>();
    std::cout << task->describe() << '\n';

    SilentTask silent;
    std::cout << silent.describe() << '\n';

    std::cout << std::boolalpha
              << "Task abstract=" << std::is_abstract_v<Task>
              << "，StillAbstract abstract=" << std::is_abstract_v<StillAbstract>
              << "，RetryTask abstract=" << std::is_abstract_v<RetryTask> << '\n';

    static_assert(std::is_abstract_v<Task>);
    static_assert(std::is_abstract_v<StillAbstract>);
    static_assert(!std::is_abstract_v<RetryTask>);
    static_assert(std::has_virtual_destructor_v<Task>);
    static_assert(std::is_nothrow_destructible_v<Task>);
}
