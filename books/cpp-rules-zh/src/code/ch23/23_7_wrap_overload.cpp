// 第 23.7 节：用 lambda 包一层重载名；标准库成员函数的地址不可移植
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

struct Student {
    std::string name;
    int midterm;
    int final;
    int homework;
};

double grade(const Student& student) {
    return 0.4 * student.midterm + 0.4 * student.final + 0.2 * student.homework;
}

double grade(int midterm, int final, int homework) {
    return 0.4 * midterm + 0.4 * final + 0.2 * homework;
}

int main() {
    std::vector<Student> students{{"Ann", 90, 80, 100}, {"Bo", 60, 70, 80}};
    std::vector<double> grades;

    // lambda 的函数体里是一次普通调用，重载决议按实参正常进行
    std::transform(students.begin(), students.end(), std::back_inserter(grades),
                   [](const Student& student) { return grade(student); });

    // 另一种写法：先转成签名明确的函数指针
    double (*recordGrade)(const Student&) =
        static_cast<double (*)(const Student&)>(grade);

    std::printf("Ann = %.1f，Bo = %.1f\n", grades[0], grades[1]);
    std::printf("用函数指针再算 Ann = %.1f\n", recordGrade(students[0]));

#if defined(__GLIBCXX__)
    // 本机 libstdc++ 15 的 clear() 恰好没有额外参数，所以这里能编过。
    // 标准允许实现给成员函数加默认参数，换实现或版本，签名就可能不同。
    void (std::vector<int>::*clearFn)() = &std::vector<int>::clear;
    std::vector<int> numbers{1, 2, 3};
    (numbers.*clearFn)();
    std::printf("取标准库成员函数地址：本机可以，清空后 size = %zu\n", numbers.size());
#else
    std::puts("本机标准库不是 libstdc++，跳过取标准库成员函数地址的演示");
#endif

    // 可移植的写法：包一层 lambda，不关心成员函数的确切签名
    auto clearIt = [](auto& container) { container.clear(); };
    std::vector<int> more{1, 2, 3};
    clearIt(more);
    std::printf("lambda 清空后 size = %zu\n", more.size());
}
