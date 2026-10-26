// 第 4.5 节：goto 和 switch 不能跳过非平凡对象的初始化
// mode: fail
// expect: cannot jump
#include <string>

void jump_over_string(bool skip) {
    if (skip) {
        goto done;                 // 想跳过 message 的构造
    }
    std::string message = "ready";
done:
    (void)message;
}

void switch_into_scope(int code) {
    switch (code) {
    case 1:
        std::string name = "one";   // 没有花括号，name 的作用域延伸到 switch 末尾
        (void)name;
        break;
    case 2:
        (void)name;                 // 从 case 2 跳进来会绕过 name 的初始化
        break;
    }
}

int main() {
    jump_over_string(true);
    switch_into_scope(2);
}
