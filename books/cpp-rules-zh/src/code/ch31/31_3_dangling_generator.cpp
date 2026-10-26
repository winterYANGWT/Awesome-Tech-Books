// 第 31.3 节：生成器只保存引用，底层容器先销毁，恢复时就访问了已释放的内存
// mode: crash
// expect: heap-use-after-free
// std: c++23
#include <generator>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

std::generator<const std::string&> names(const std::vector<std::string>& source) {
    for (const std::string& name : source) {
        co_yield name;
    }
}

int main() {
    auto owned = std::make_unique<std::vector<std::string>>();
    owned->push_back("first-customer-name");
    owned->push_back("second-customer-name");

    auto stream = names(*owned);
    std::cout << "generator created; the vector is still alive\n";
    owned.reset();
    std::cout << "vector destroyed; now iterate\n";

    for (const std::string& name : stream) {
        std::cout << "name: " << name << "\n";
    }
}
