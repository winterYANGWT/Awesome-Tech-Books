// 第 22.3 节：把 is_constant_evaluated() 写进 if constexpr，条件永远是 true
// mode: warn
// expect: -Wconstant-evaluated
#include <type_traits>

constexpr bool use_compile_time_path() {
    // if constexpr 的条件本身就是“明显常量求值”的上下文，
    // 所以 is_constant_evaluated() 在这里恒为 true，运行期也走这一支。
    if constexpr (std::is_constant_evaluated())
        return true;
    else
        return false;
}

int main() {
    static_assert(use_compile_time_path());
}
