// 第 27.4 节：比较器传函数对象或 lambda，而不是裸函数指针
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

bool by_length_fn(const std::string& a, const std::string& b) {
    return a.size() < b.size();
}

struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() < b.size();
    }
};

template <class Vec>
void show_lengths(const std::string& label, const Vec& v) {
    std::cout << label;
    for (const auto& s : v) std::cout << ' ' << s.size();
    std::cout << '\n';
}

int main() {
    std::vector<std::string> words{"pear", "fig", "banana", "kiwi", "apple"};

    auto with_fn = words;
    std::sort(with_fn.begin(), with_fn.end(), by_length_fn);

    auto with_functor = words;
    std::sort(with_functor.begin(), with_functor.end(), ByLength{});

    auto with_lambda = words;
    std::sort(with_lambda.begin(), with_lambda.end(),
              [](const auto& a, const auto& b) { return a.size() < b.size(); });

    show_lengths("函数名：  ", with_fn);
    show_lengths("函数对象：", with_functor);
    show_lengths("lambda：  ", with_lambda);

    // 函数名作实参时退化为函数指针；函数对象和 lambda 都是各自不同的类类型。
    static_assert(std::is_pointer_v<decltype(&by_length_fn)>);
    static_assert(!std::is_pointer_v<ByLength>);
    static_assert(!std::is_pointer_v<decltype([](const auto&, const auto&) { return true; })>);

    // std::function 的类型擦除又会引入一层间接调用，能不用就不用。
    std::function<bool(const std::string&, const std::string&)> erased = ByLength{};
    std::cout << "std::function 也能调用：" << erased("aa", "b") << '\n';
}
