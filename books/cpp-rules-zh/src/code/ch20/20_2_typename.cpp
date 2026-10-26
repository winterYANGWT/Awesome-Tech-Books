// 第 20.2 节：嵌套依赖类型名要写 typename，长名字先用 typedef 收短
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <type_traits>
#include <vector>

template<class C>
struct SequenceView {
    using value_type = typename C::value_type;
    using const_iterator = typename C::const_iterator;

    static value_type second(const C& c) {
        typename C::const_iterator it = c.begin();  // 变量声明：typename 不能省
        ++it;
        return *it;
    }
};

// 标准写法：先用 typedef 把 traits 里的依赖名变成普通名字
template<class It>
typename std::iterator_traits<It>::value_type nth_value(It it, int n) {
    using value_type = typename std::iterator_traits<It>::value_type;
    value_type temp{};
    for (int i = 0; i < n; ++i) {
        ++it;
    }
    temp = *it;
    return temp;
}

// C++20（P0634R3）起，这些位置不再要求写 typename
template<class T>
struct Wrapper {
    using inner = T::value_type;       // 别名声明
    T::value_type first() const;       // 返回类型
    void take(T::value_type);          // 参数声明
};

using IntView = Wrapper<std::vector<int>>;
static_assert(std::is_same_v<IntView::inner, int>);

int main() {
    std::vector<int> numbers{10, 20, 30};
    std::list<std::string> words{"alpha", "beta", "gamma"};

    std::cout << "vector second = " << SequenceView<std::vector<int>>::second(numbers) << '\n';
    std::cout << "list second = " << SequenceView<std::list<std::string>>::second(words) << '\n';
    std::cout << "nth_value = " << nth_value(numbers.begin(), 2) << '\n';
}
