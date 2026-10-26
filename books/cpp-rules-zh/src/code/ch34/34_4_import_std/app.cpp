// 第 34.4 节：C++23 的 import std; 把标准库也做成一个模块
import std;

int main() {
    std::vector<int> values{3, 1, 4, 1, 5};
    std::ranges::sort(values);
    std::cout << std::format("{} 个值，最大 {}\n", values.size(), values.back());
}
