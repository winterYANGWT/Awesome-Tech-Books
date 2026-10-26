// 第 27.3 节：手写循环在遍历时改容器，迭代器失效往往要等到运行期才暴露
// mode: crash
// expect: heap-use-after-free
#include <iostream>
#include <vector>

int main() {
    std::vector<int> values{1, 2, 3, 4};
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (*it % 2 == 0) values.push_back(*it * 10);  // 扩容后 it 失效
    }
    for (int v : values) std::cout << v << ' ';
    std::cout << '\n';
}
