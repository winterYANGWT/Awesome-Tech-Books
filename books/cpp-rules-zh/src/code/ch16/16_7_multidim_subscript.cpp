// 第 16.7 节：C++23 的 operator[] 可以收多个参数，也可以声明为 static
// std: c++23
#include <array>
#include <iostream>

class Grid {
public:
    static constexpr int rows = 2;
    static constexpr int cols = 3;

    int& operator[](int row, int col) { return cells_[row][col]; }
    int operator[](int row, int col) const { return cells_[row][col]; }

    // 需要兼容 C++20 时，保留一个具名或 operator() 入口
    int& at(int row, int col) { return cells_[row][col]; }

private:
    std::array<std::array<int, cols>, rows> cells_{{{1, 2, 3}, {4, 5, 6}}};
};

struct Axis {
    // static operator[] 不依赖对象状态，只按参数返回结果
    static constexpr int operator[](int index) { return index == 0 ? 1 : 0; }
};

int main() {
    Grid grid;
    grid[1, 2] = 42;              // C++23：两个下标参数，不再写成 grid[1][2]
    std::cout << "grid[1, 2] = " << grid[1, 2] << '\n';
    std::cout << "grid.at(0, 1) = " << grid.at(0, 1) << '\n';

    std::cout << "Axis{}[0] = " << Axis{}[0] << '\n';
    std::cout << "Axis{}[1] = " << Axis{}[1] << '\n';
}
