// 第 28.7 节：流迭代器构造即预读、默认构造是 end，以及 endl 的 flush 代价
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

class CountingBuf : public std::streambuf {
public:
    int syncCount() const { return syncs_; }
    int charCount() const { return chars_; }

protected:
    int overflow(int c) override {
        if (c != traits_type::eof()) {
            ++chars_;
            return c;
        }
        return traits_type::not_eof(c);
    }
    int sync() override {
        ++syncs_;
        return 0;
    }

private:
    int syncs_ = 0;
    int chars_ = 0;
};

int main() {
    std::istringstream input("10 20 30");
    std::istream_iterator<int> it(input);
    const std::istream_iterator<int> end;

    // 构造时已经执行了一次 >>，所以还没 ++ 就能读到第一个值。
    std::cout << "构造后立即 *it = " << *it << '\n';

    std::vector<int> values;
    std::copy(it, end, std::back_inserter(values));
    std::cout << "copy 得到:";
    for (int v : values) {
        std::cout << ' ' << v;
    }
    std::cout << "，读到默认构造的 end 后停止\n";

    // ostream_iterator 把输出接进算法，第二个参数是每个元素后的分隔符。
    std::ostringstream formatted;
    std::ostream_iterator<int> out(formatted, ", ");
    std::copy(values.begin(), values.end(), out);
    std::cout << "ostream_iterator: [" << formatted.str() << "]\n";

    CountingBuf buf;
    std::ostream os(&buf);
    os << "a" << '\n';
    std::cout << "写 '\\n' 后 sync 次数=" << buf.syncCount()
              << "，字符数=" << buf.charCount() << '\n';
    os << "b" << std::endl;
    std::cout << "写 endl 后 sync 次数=" << buf.syncCount()
              << "，字符数=" << buf.charCount() << '\n';
}
