// 第 23.7 节：重载函数名不能直接当可调用对象传给算法
// mode: fail
// expect: couldn't infer
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

struct Student {
    std::string name;
    int midterm;
    int final;
    int homework;
};

double grade(const Student& student);
double grade(int midterm, int final, int homework);

int main() {
    std::vector<Student> students{{"Ann", 90, 80, 100}};
    std::vector<double> grades;

    // 没有实参可供重载决议，编译器不知道要哪一个 grade
    std::transform(students.begin(), students.end(),
                   std::back_inserter(grades), grade);
}
