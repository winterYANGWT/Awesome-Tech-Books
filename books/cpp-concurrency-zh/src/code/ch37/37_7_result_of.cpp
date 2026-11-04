// mode: compile
// std: c++20
// libstdc++ 15 仍保留 std::result_of 作兼容名；C++20 已把它从标准移除。
#include <type_traits>

static int add(int a, int b) { return a + b; }

using old_type = std::result_of<decltype(&add)(int, int)>::type;          // 旧写法
using new_type = std::invoke_result_t<decltype(&add), int, int>;          // C++17 起

static_assert(std::is_same_v<old_type, new_type>);

int main() {
    return 0;
}
