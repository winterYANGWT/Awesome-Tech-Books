// 第 4.4 节：C++20 指定初始化器要按声明顺序写；clang 默认只警告，GCC 直接报错
// mode: warn
// expect: -Wreorder-init-list
struct Window {
    int width;
    int height;
};

int main() {
    Window reordered{.height = 600, .width = 800};   // 顺序反了：标准规定 ill-formed
    Window mixed{.width = 800, 600};                  // 指定和位置混用：同样 ill-formed
    return reordered.width + mixed.height;
}
