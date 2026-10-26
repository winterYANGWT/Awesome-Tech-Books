// 第 27.4 节：谓词携带可变状态时，算法拿到的副本会各走各的
#include <algorithm>
#include <iostream>
#include <vector>

struct NthCall {
    int n;
    mutable int seen = 0;

    bool operator()(int) const { return ++seen == n; }
};

int main() {
    // 语言层面先看清楚：拷贝出来的谓词各有自己的计数器。
    NthCall p{3};
    NthCall q = p;
    std::cout << "p(1)=" << p(1) << "，q(1)=" << q(1)
              << "，q(2)=" << q(2) << "，q(3)=" << q(3) << '\n';

    std::vector<int> values{1, 2, 3, 4, 5, 6, 7, 8, 9};

    NthCall find_pred{3};
    auto found = std::find_if(values.begin(), values.end(), find_pred);
    std::cout << "find_if 找到 " << *found
              << "，调用后 find_pred.seen=" << find_pred.seen << '\n';

    // 标准不保证 remove_if 用哪一份谓词、按什么顺序调用；
    // 下面是本机 libstdc++ 15 的实际结果，换个实现可能不同。
    NthCall remove_pred{3};
    auto new_end = std::remove_if(values.begin(), values.end(), remove_pred);
    std::cout << "remove_if 之后剩下：";
    for (auto it = values.begin(); it != new_end; ++it) std::cout << ' ' << *it;
    std::cout << "；调用后 remove_pred.seen=" << remove_pred.seen << '\n';
}
