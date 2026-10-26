// 第 29.2 节：C++20 的 views 把过滤和变换推迟到真正迭代时
#include <iostream>
#include <ranges>
#include <vector>

int main() {
    std::vector<int> data(1000);
    for (int i = 0; i < 1000; ++i) data[static_cast<std::size_t>(i)] = i;

    int filter_calls = 0;
    int transform_calls = 0;

    auto view = data
              | std::views::filter([&](int x) {
                    ++filter_calls;
                    return x % 2 == 0;
                })
              | std::views::transform([&](int x) {
                    ++transform_calls;
                    return x * 10;
                });

    std::cout << "构造 view 之后: filter=" << filter_calls
              << " transform=" << transform_calls << '\n';

    std::cout << "只取前 5 个:";
    for (int value : view | std::views::take(5)) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';
    std::cout << "取前 5 个之后: filter=" << filter_calls
              << " transform=" << transform_calls << '\n';

    // 同样的过滤和变换，用 eager 循环一次做完：所有元素都要过一遍。
    filter_calls = 0;
    transform_calls = 0;
    std::vector<int> eager;
    for (int x : data) {
        ++filter_calls;              // 谓词对每个元素都要判断一次
        if (x % 2 == 0) {
            eager.push_back(x * 10);
            ++transform_calls;
        }
    }
    std::cout << "eager 全量: filter=" << filter_calls
              << " transform=" << transform_calls
              << " 结果=" << eager.size() << '\n';
}
