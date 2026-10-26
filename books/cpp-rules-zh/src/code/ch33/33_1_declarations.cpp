// 第 33.1 节：前置声明让头文件只依赖名字，不依赖定义
#include <iostream>
#include <type_traits>

struct Report;                // 只要名字，不要定义
Report make_report();         // 按值返回的声明也可以只看到前置声明
void publish(const Report&);  // 引用参数
void archive(Report*);        // 指针参数

static_assert(std::is_same_v<decltype(make_report()), Report>);

struct Report {               // 定义可以晚到，放在 .cpp 里也一样
    int lines;
};

class Dashboard {
public:
    explicit Dashboard(Report* report) : report_(report) {}
    int lines() const;        // 定义要等 Report 完整
private:
    Report* report_;          // 指针成员只需要前置声明
};

int Dashboard::lines() const { return report_->lines; }

Report make_report() { return Report{2}; }
void publish(const Report&) {}
void archive(Report*) {}

int main() {
    Report report = make_report();
    Dashboard dashboard(&report);
    std::cout << "lines=" << dashboard.lines() << '\n';
    std::cout << "pointer declaration is enough: "
              << (sizeof(Report*) == sizeof(void*) ? "yes" : "no") << '\n';
}
