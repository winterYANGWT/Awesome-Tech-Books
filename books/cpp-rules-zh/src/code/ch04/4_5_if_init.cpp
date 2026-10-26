// 第 4.5 节：C++17 的 if/switch 初始化语句，和 while 条件里声明的老陷阱
#include <iostream>
#include <map>
#include <sstream>
#include <string>

int read_command() {
    return 2;
}

int main() {
    std::map<std::string, int> stock{{"apple", 3}, {"pear", 5}};

    // C++17：it 只活在 if 语句里，else 分支也看得到
    if (auto it = stock.find("pear"); it != stock.end()) {
        std::cout << "if init: " << it->first << " = " << it->second << "\n";
    } else {
        std::cout << "if init: 没找到\n";
    }

    // switch 也可以先初始化一个变量，再拿它做条件
    switch (int command = read_command(); command) {
    case 1:
        std::cout << "switch init: one\n";
        break;
    case 2:
        std::cout << "switch init: two\n";
        break;
    default:
        std::cout << "switch init: other\n";
        break;
    }

    // 老陷阱：!= 先算，c 得到的是 bool 转成的 char，不是读到的字符
    std::istringstream input("abcq");
    std::cout << "while 条件里声明 c: ";
    while (char c = input.get() != 'q') {
        std::cout << static_cast<int>(c) << " ";
    }
    std::cout << "\n";

    // while 没有 init-statement；把读取放进循环体，语义才清楚
    std::istringstream input2("abcq");
    std::cout << "循环体里读取: ";
    for (char c; input2.get(c) && c != 'q';) {
        std::cout << c << " ";
    }
    std::cout << "\n";
}
