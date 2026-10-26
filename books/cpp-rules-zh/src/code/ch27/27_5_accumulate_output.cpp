// 第 27.5 节：归约成一个值，以及用输出迭代器决定结果写到哪里
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

struct Average {
    double sum = 0.0;
    int count = 0;

    void operator()(double x) {
        sum += x;
        ++count;
    }

    double mean() const { return count == 0 ? 0.0 : sum / count; }
};

// 只依赖输出迭代器的最少操作：*out++ = value。
template <class Out>
void split_words(const std::string& text, Out out) {
    std::istringstream in(text);
    std::string word;
    while (in >> word) *out++ = word;
}

int main() {
    std::vector<double> prices{19.99, 5.50, 3.25};

    // 初值的类型就是累加器的类型：0 会让每次加法后截断成 int。
    int wrong = std::accumulate(prices.begin(), prices.end(), 0);
    double right = std::accumulate(prices.begin(), prices.end(), 0.0);
    std::cout << std::fixed << std::setprecision(2)
              << "accumulate 初值 0（int）：" << wrong << '\n'
              << "accumulate 初值 0.0（double）：" << right << '\n';

    std::vector<std::string> colors{"red", "green", "blue", "cyan"};
    auto length_sum = std::accumulate(
        colors.begin(), colors.end(), std::size_t{0},
        [](std::size_t n, const std::string& s) { return n + s.size(); });
    std::cout << "accumulate 字符串长度和：" << length_sum << '\n';

    // 需要副作用或“边遍历边维护多个量”时，for_each 更自然。
    Average avg = std::for_each(prices.begin(), prices.end(), Average{});
    std::cout << "for_each 平均：" << avg.mean() << '\n';

    // 同一个 split_words，调用方决定结果写进哪里。
    std::vector<std::string> words;
    split_words("the quick brown fox", std::back_inserter(words));
    std::cout << "split 到 vector：" << words.size() << " 个词：";
    for (const auto& w : words) std::cout << w << ' ';
    std::cout << '\n';

    std::cout << "split 到 ostream：";
    split_words("alpha beta", std::ostream_iterator<std::string>(std::cout, "|"));
    std::cout << '\n';
}
