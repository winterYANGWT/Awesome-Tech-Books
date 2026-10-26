// 第 32.1 节：if constexpr 的弃用分支仍要能通过编译，不像 #if 0 那样藏起错误
// mode: fail
// expect: no_such_function
int main() {
    if constexpr (false) {
        no_such_function();  // 非模板代码里，这个分支仍会被检查
    }
}
