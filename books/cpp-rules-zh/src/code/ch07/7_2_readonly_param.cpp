// 第 7.2 节：只读参数用 const&，既避免拷贝也避免切片
#include <cstddef>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

struct Employee {
    static int copies;
    std::string name;

    Employee() = default;
    explicit Employee(std::string n) : name(std::move(n)) {}
    Employee(const Employee& other) : name(other.name) { ++copies; }
    virtual ~Employee() = default;

    virtual std::string role() const { return "employee"; }
};
int Employee::copies = 0;

struct Manager : Employee {
    std::size_t team_size = 0;

    Manager(std::string n, std::size_t team)
        : Employee(std::move(n)), team_size(team) {}

    std::string role() const override {
        return "manager of " + std::to_string(team_size);
    }
};

void print_badge_by_value(Employee person) {
    std::cout << "按值传参：" << person.name << " -> " << person.role() << "\n";
}

void print_badge_by_ref(const Employee& person) {
    std::cout << "const& 传参：" << person.name << " -> " << person.role() << "\n";
}

struct Matrix {
    static int copies;
    static long long values_copied;
    std::vector<double> cells;

    explicit Matrix(std::size_t n) : cells(n, 1.0) {}
    Matrix(const Matrix& other) : cells(other.cells) {
        ++copies;
        values_copied += static_cast<long long>(cells.size());
    }
};
int Matrix::copies = 0;
long long Matrix::values_copied = 0;

double sum_by_value(Matrix matrix) {
    return std::accumulate(matrix.cells.begin(), matrix.cells.end(), 0.0);
}

double sum_by_ref(const Matrix& matrix) {
    return std::accumulate(matrix.cells.begin(), matrix.cells.end(), 0.0);
}

int main() {
    Manager ada("Ada", 3);
    Employee::copies = 0;
    print_badge_by_value(ada);
    std::cout << "按值调用后 Employee 拷贝 " << Employee::copies << " 次\n";

    Employee::copies = 0;
    print_badge_by_ref(ada);
    std::cout << "const& 调用后 Employee 拷贝 " << Employee::copies << " 次\n";

    // 对象本身很小，但它拥有的 vector 很大；拷贝成本由元素决定。
    Matrix grid(10000);
    std::cout << "Matrix 对象 " << sizeof(Matrix) << " 字节，元素 "
              << grid.cells.size() << " 个\n";

    Matrix::copies = 0;
    Matrix::values_copied = 0;
    double a = sum_by_value(grid);
    std::cout << "按值求和 " << a << "：拷贝 " << Matrix::copies
              << " 次，复制 " << Matrix::values_copied << " 个 double\n";

    Matrix::copies = 0;
    Matrix::values_copied = 0;
    double b = sum_by_ref(grid);
    std::cout << "const& 求和 " << b << "：拷贝 " << Matrix::copies
              << " 次，复制 " << Matrix::values_copied << " 个 double\n";
}
