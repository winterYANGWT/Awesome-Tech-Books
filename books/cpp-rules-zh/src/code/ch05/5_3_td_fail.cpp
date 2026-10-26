// 第 5.3 节：用故意不完整的 TD<T> 让编译器在错误信息里打出推导结果
// mode: fail
// expect: TD<int>
#include <string>

template <typename T>
class TD;   // 只声明，不定义；实例化它必然出错

int main() {
    int count = 3;
    const std::string& name = "Ada";

    TD<decltype(count)> countType;   // 错误信息里会出现 TD<int>
    TD<decltype(name)> nameType;     // 错误信息里会出现 TD<const std::string&>
}
