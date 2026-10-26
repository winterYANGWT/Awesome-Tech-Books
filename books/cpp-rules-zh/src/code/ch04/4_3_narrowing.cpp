// 第 4.3 节：花括号初始化拒绝窄化，圆括号和等号会静默截断
// mode: fail
// expect: narrow
int main() {
    double measured = 3.9;
    int rounded{measured};   // double 的和不一定能用 int 表示，花括号直接拒绝
    return rounded;
}
