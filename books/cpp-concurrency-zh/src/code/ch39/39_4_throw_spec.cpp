// mode: fail
// expect: dynamic exception specifications
// C++17 起移除动态异常规格；新代码用 noexcept。
void f() throw(int) {}

int main() {
    f();
}
