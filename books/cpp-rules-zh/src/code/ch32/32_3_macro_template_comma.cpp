// 第 32.3 节：预处理器只配平圆括号，模板的尖括号配不平
// mode: fail
// expect: arguments
#include <utility>

#define DECLARE_PAIR(type, name) type name

int main() {
    // 宏看到 std::pair<int, int> 里的逗号，以为传了三个实参。
    DECLARE_PAIR(std::pair<int, int>, p) {1, 2};
    return p.first;
}
