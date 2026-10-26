// 第 28.2 节：把 vector/string 的连续数据交给 C API，并守住 size 边界
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// 模拟一个遗留 C 接口：最多写 max 个 int，返回实际写入个数。
extern "C" std::size_t fill_ids(int* out, std::size_t max) {
    const int source[] = {101, 102, 103, 104, 105};
    std::size_t n = 0;
    while (n < max && n < sizeof(source) / sizeof(source[0])) {
        out[n] = source[n];
        ++n;
    }
    return n;
}

// 只读接口：长度由调用方另传，函数不会猜。
extern "C" long sum_ids(const int* values, std::size_t count) {
    long total = 0;
    for (std::size_t i = 0; i < count; ++i) {
        total += values[i];
    }
    return total;
}

int main() {
    // 输出方向：先准备足够大的 size，让 C API 只改已有元素，回来再 resize。
    std::vector<int> ids(8);
    const std::size_t written = fill_ids(ids.data(), ids.size());
    ids.resize(written);
    std::cout << "C API 写入 " << ids.size() << " 个，和=" << sum_ids(ids.data(), ids.size())
              << '\n';

    // 输入方向：容器直接给出 data() 和 size()，C 函数按 size 读。
    const std::vector<int> fixed{7, 8, 9};
    std::cout << "只读传入 " << fixed.size() << " 个，和=" << sum_ids(fixed.data(), fixed.size())
              << '\n';

    // 空容器：data() 可能为空指针，但长度是 0，合法调用不会解引用它。
    const std::vector<int> empty;
    std::cout << "空 vector 的和=" << sum_ids(empty.data(), empty.size()) << '\n';

    // C API 只认到第一个 '\0'，string 自己知道真实长度。
    const std::string record("Ada\0Lovelace", 10);
    std::cout << "string::size=" << record.size()
              << "，strlen(c_str())=" << std::strlen(record.c_str()) << '\n';

    // C++17 起 data() 有可写重载；可以改已有字符，但不能借此改 size。
    std::string code = "abc";
    code.data()[0] = 'A';
    std::cout << "改写已有字符后: " << code << "，size=" << code.size() << '\n';
}
