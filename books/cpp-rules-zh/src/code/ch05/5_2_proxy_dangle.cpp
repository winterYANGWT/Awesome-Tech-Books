// 第 5.2 节：把临时 vector<bool> 的代理对象存进 auto，临时对象销毁后代理悬垂
// mode: crash
// expect: heap-use-after-free
#include <iostream>
#include <vector>

std::vector<bool> feature_flags() {
    return {false, false, true, false, true, false, false, true};
}

bool high_priority() {
    auto proxy = feature_flags()[5];   // 临时 vector 在这条语句结束时销毁
    return proxy;                      // proxy 指向已释放的位图
}

int main() {
    std::cout << "准备读取悬垂代理\n";
    bool value = high_priority();
    std::cout << "value = " << value << "\n";
}
