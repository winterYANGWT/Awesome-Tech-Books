// 第 24.6 节：容器可以存 move-only 类型，但插入左值仍要求可拷贝
// mode: fail
// expect: unique_ptr
#include <memory>
#include <vector>

int main() {
    std::vector<std::unique_ptr<int>> values;
    auto number = std::make_unique<int>(42);

    values.push_back(number);  // unique_ptr 不可拷贝，这里编不过

    // 正确写法：values.push_back(std::move(number));
}
