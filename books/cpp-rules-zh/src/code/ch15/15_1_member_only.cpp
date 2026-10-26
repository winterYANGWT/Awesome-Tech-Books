// 第 15.1 节：语言规定这几个运算符必须是成员，写成自由函数编译不过
// mode: fail
// expect: must be a non-static member function
#include <cstddef>

struct Grid {
    int cells[4]{};
};

// 下面四个定义都会被拒绝：operator=、operator[]、operator()、operator->
// 只能是类的非静态成员函数，这是语法层面的强制规定。
int& operator[](Grid& g, std::size_t i) { return g.cells[i]; }

Grid& operator=(Grid& lhs, const Grid& rhs) {
    for (int i = 0; i < 4; ++i) {
        lhs.cells[i] = rhs.cells[i];
    }
    return lhs;
}

int operator()(const Grid& g, std::size_t i) { return g.cells[i]; }

int* operator->(Grid& g) { return g.cells; }

int main() {}
