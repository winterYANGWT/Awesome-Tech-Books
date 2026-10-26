// 第 28.8 节：C++11 起字符串字面量不能再隐式转成可写的 char*
// mode: fail
// flags: -Werror=write-strings
// expect: string literal
int main() {
    char* p = "hello";
    p[0] = 'H';
}
