// 第 14.3 节：= delete 的函数仍参与重载决议，被选中才报错
// mode: fail
// expect: deleted
#include <iostream>

class Logger {
public:
    void log(double value) { std::cout << "double " << value << "\n"; }
    void log(int) = delete;   // 禁止整数，但名字仍然可见
};

int main() {
    Logger l;
    l.log(1);   // 选中被删除的 int 重载，而不是退到 double
}
