// 第 19.4 节：接口类加句柄类，用虚 clone 给多态对象值语义
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Student {
public:
    virtual ~Student() = default;
    virtual std::string name() const = 0;
    virtual int grade() const = 0;
    virtual std::unique_ptr<Student> clone() const = 0;
};

class Undergraduate : public Student {
public:
    Undergraduate(std::string name, int grade)
        : name_(std::move(name)), grade_(grade) {}

    std::string name() const override { return name_; }
    int grade() const override { return grade_; }

    std::unique_ptr<Student> clone() const override {
        std::cout << "  clone Undergraduate\n";
        return std::make_unique<Undergraduate>(*this);
    }

private:
    std::string name_;
    int grade_;
};

class Graduate : public Student {
public:
    Graduate(std::string name, int grade)
        : name_(std::move(name)), grade_(grade) {}

    std::string name() const override { return name_; }
    int grade() const override { return grade_; }

    std::unique_ptr<Student> clone() const override {
        std::cout << "  clone Graduate\n";
        return std::make_unique<Graduate>(*this);
    }

private:
    std::string name_;
    int grade_;
};

// 句柄类只管所有权和复制，业务接口仍然由 Student 决定。
class StudentHandle {
public:
    StudentHandle() = default;
    explicit StudentHandle(std::unique_ptr<Student> student)
        : student_(std::move(student)) {}

    StudentHandle(const StudentHandle& other)
        : student_(other.student_ ? other.student_->clone() : nullptr) {}

    StudentHandle& operator=(const StudentHandle& other) {
        if (this != &other) {
            StudentHandle copy(other);
            swap(copy);
        }
        return *this;
    }

    StudentHandle(StudentHandle&&) noexcept = default;
    StudentHandle& operator=(StudentHandle&&) noexcept = default;
    ~StudentHandle() = default;

    const Student& operator*() const { return *student_; }
    const Student* operator->() const { return student_.get(); }
    explicit operator bool() const { return student_ != nullptr; }

    void swap(StudentHandle& other) noexcept { student_.swap(other.student_); }

private:
    std::unique_ptr<Student> student_;
};

int main() {
    StudentHandle first(std::make_unique<Undergraduate>("Ada", 90));
    StudentHandle second(std::make_unique<Graduate>("Grace", 95));

    std::cout << "赋值（按动态类型克隆）：\n";
    first = second;
    std::cout << "first 现在是 " << first->name() << ": " << first->grade() << '\n';

    std::vector<StudentHandle> roster;
    roster.push_back(StudentHandle(std::make_unique<Undergraduate>("Ada", 90)));
    roster.push_back(StudentHandle(std::make_unique<Graduate>("Grace", 95)));

    std::cout << "复制 vector：\n";
    std::vector<StudentHandle> copy = roster;
    roster.clear();

    std::cout << "原 vector 清空后，copy 仍然完整：\n";
    for (const auto& student : copy) {
        std::cout << "  " << student->name() << ": " << student->grade() << '\n';
    }
}
