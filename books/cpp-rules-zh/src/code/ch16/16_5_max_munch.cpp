// 第 16.5 节：最长匹配把连续加号切成 ++ 和 +，>> 也不再需要空格
#include <iostream>
#include <vector>

class Tally {
public:
    explicit Tally(int value) : value_(value) {}

    Tally& operator++() {
        ++value_;
        return *this;
    }

    // 一元 +：只为演示奇数个加号时的切分，没有实际语义
    Tally& operator+() { return *this; }

    int value() const { return value_; }

private:
    int value_;
};

int main() {
    Tally six(0);
    ++++++six;                // 6 个加号 -> ++ ++ ++ six
    std::cout << "++++++six: " << six.value() << '\n';

    Tally seven(0);
    +++++++seven;             // 7 个加号 -> ++ ++ ++ + seven
    std::cout << "+++++++seven: " << seven.value() << '\n';

    // C++11 起，嵌套模板的 >> 直接连写即可，编译器知道它在闭合模板参数
    std::vector<std::vector<int>> grid{{1, 2}, {3}};
    std::cout << "grid[0][1]: " << grid[0][1] << '\n';
}
