// 第 33.3 节：不要向 std 添加声明，也不要手写标准库类型的前置声明
// mode: fail
// expect: incomplete
namespace std { class string; }   // 未定义行为：向 std 添加声明

int main() {
    std::string name = "ada";      // string 仍然是不完整类型
}
