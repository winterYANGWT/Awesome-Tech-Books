// 第 20.6 节：把比较算法作为模板参数，C++20 起可以把约定写成 concept
#include <concepts>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>

template<class C, class T>
concept Comparator = requires(const C& comp, const T& a, const T& b) {
    { comp(a, b) } -> std::convertible_to<bool>;
};

template<class T, Comparator<T> Comp = std::less<T>>
class Range {
public:
    Range(T low, T high) : low_(low), high_(high) {}

    bool contains(const T& value) const {
        Comp less{};
        return !less(value, low_) && !less(high_, value);
    }

private:
    T low_;
    T high_;
};

struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() < b.size();
    }
};

struct Bad {
    void operator()(const std::string&, const std::string&) const {}
};

static_assert(Comparator<ByLength, std::string>);
static_assert(!Comparator<Bad, std::string>);
static_assert(!std::is_same_v<Range<int>, Range<int, std::greater<int>>>);

int main() {
    Range<int> numbers(10, 20);
    std::cout << std::boolalpha
              << "15 in [10,20]: " << numbers.contains(15) << '\n'
              << "25 in [10,20]: " << numbers.contains(25) << '\n';

    Range<std::string, ByLength> lengths("aa", "dddd");
    std::cout << "'bbb' in length range: " << lengths.contains("bbb") << '\n'
              << "'eeeee' in length range: " << lengths.contains("eeeee") << '\n';

    Range<int, std::greater<int>> inverted(20, 10);
    std::cout << "15 in inverted range: " << inverted.contains(15) << '\n';
}
