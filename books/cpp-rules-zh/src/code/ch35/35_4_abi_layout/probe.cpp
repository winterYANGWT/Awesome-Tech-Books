// 第 35.4 节：同一个标准库，换一个构建选项就换一套标准库类型布局
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::cout << "string=" << sizeof(std::string)
              << " vector=" << sizeof(std::vector<int>) << '\n';
}
