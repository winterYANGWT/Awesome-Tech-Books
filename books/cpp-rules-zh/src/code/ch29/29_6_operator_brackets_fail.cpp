// 第 29.6 节：C++ 没有 operator[][]，二维下标要靠代理类
// mode: fail
#include <cstddef>

template <class T>
class Grid {
public:
    // 语言里不存在 operator[][]；下面这行不是合法的声明。
    T& operator[][](int row, int col);
};

int main() {
    Grid<int> grid;
    (void)grid;
}
