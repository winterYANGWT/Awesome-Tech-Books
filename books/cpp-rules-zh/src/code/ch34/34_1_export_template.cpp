// 第 34.1 节：export 模板在 C++11 已被删除，现代编译器只把它当历史语法
// mode: warn
// std: c++17
// expect: exported templates are unsupported

export template <typename T>
T twice(T value) {
    return value + value;
}

int main() {
    return twice(21) == 42 ? 0 : 1;
}
