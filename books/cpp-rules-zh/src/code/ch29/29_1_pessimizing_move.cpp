// 第 29.1 节：对局部变量写 return std::move 会妨碍 NRVO
// mode: warn
// expect: moving a local object
#include <string>

std::string make_name() {
    std::string name(100, 'x');
    return std::move(name);   // name 是局部变量，这里应直接 return name;
}

int main() {
    return make_name().size() == 100 ? 0 : 1;
}
