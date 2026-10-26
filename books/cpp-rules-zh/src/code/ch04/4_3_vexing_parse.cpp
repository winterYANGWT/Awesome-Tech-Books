// 第 4.3 节：能被解析成函数声明的语句就是声明
// flags: -Wno-vexing-parse
// 关掉警告只是为了让下面两行“陷阱”能跑起来；真实代码应当改写，而不是关警告。
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

struct Report {
    std::string title;
};

int main() {
    // 这一行声明的是函数 r，返回 Report、没有参数
    Report r();
    static_assert(std::is_same_v<decltype(r), Report()>,
                  "r 的类型是函数类型，不是一个 Report 对象");

    // 经典陷阱：第二个实参被读成“返回 istream_iterator<int> 的无参函数”
    std::istringstream numbers("10 20 30 40");
    std::vector<int> data(std::istream_iterator<int>(numbers),
                          std::istream_iterator<int>());
    static_assert(std::is_same_v<decltype(data),
                                 std::vector<int>(std::istream_iterator<int>,
                                                  std::istream_iterator<int> (*)())>,
                  "data 也是函数声明，没有构造任何 vector");

    // 修法一：先给迭代器名字，再传给区间构造函数
    std::istringstream named_input("10 20 30 40");
    std::istream_iterator<int> first(named_input);
    std::istream_iterator<int> last;
    std::vector<int> named(first, last);
    std::cout << "具名迭代器: " << named.size() << " 个元素:";
    for (int value : named) {
        std::cout << " " << value;
    }
    std::cout << "\n";

    // 修法二：花括号让两个实参被当作表达式
    std::istringstream braced_input("5 6 7");
    std::vector<int> braced{std::istream_iterator<int>(braced_input),
                            std::istream_iterator<int>()};
    std::cout << "花括号: " << braced.size() << " 个元素:";
    for (int value : braced) {
        std::cout << " " << value;
    }
    std::cout << "\n";
}
