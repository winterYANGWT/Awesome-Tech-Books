// 第 7.3 节：必须返回新对象时返回引用，函数一结束就悬垂
// mode: crash
// flags: -Wno-return-stack-address -Wno-return-local-addr
#include <iostream>
#include <string>

// 只为本例保留确定的诊断行，不让 ASan 在末尾打印带构建路径的摘要。
extern "C" const char* __asan_default_options() { return "print_summary=0"; }

const std::string& current_label() {
    std::string label = "draft-42";
    return label;  // 局部对象在函数返回时销毁
}

int main() {
    const std::string& ref = current_label();
    std::cout << ref << "\n";  // 读已经销毁的对象
}
