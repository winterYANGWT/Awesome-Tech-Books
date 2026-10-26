// 第 24.5 节：auto 保存 vector<bool> 的代理，临时容器销毁后代理悬垂
// mode: crash
// expect: heap-use-after-free
#include <iostream>
#include <vector>

std::vector<bool> make_flags() {
    return std::vector<bool>(64, true);
}

int main() {
    // operator[] 返回代理对象，auto 复制的是代理，不是 bool；
    // 代理里存着指向临时 vector 位存储的指针
    auto bit = make_flags()[0];
    std::cout << "临时 vector 已经销毁，代理还在\n";

    std::cout << std::boolalpha << static_cast<bool>(bit) << '\n';
}
