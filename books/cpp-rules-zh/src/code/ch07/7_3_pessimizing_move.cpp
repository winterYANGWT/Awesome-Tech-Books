// 第 7.3 节：对局部变量写 return std::move 会挡住 NRVO
// mode: warn
// expect: -Wpessimizing-move
#include <iostream>
#include <string>
#include <utility>

std::string render_label() {
    std::string label = "draft-42";
    return std::move(label);
}

int main() {
    std::cout << render_label() << "\n";
}
