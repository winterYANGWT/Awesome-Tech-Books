// 第 24.1 节：想写“什么序列容器都能用”的代码，一换容器就编不过
// mode: fail
// expect: no member named 'reserve'
#include <list>
#include <vector>

// 这段代码同时用了 reserve（只有 vector/string 有）和 push_front（只有
// deque/list 有），所以无论用哪种容器实例化都会失败
template <class Sequence>
void fill(Sequence& s) {
    s.reserve(8);       // list/deque 没有 reserve
    s.push_front(0);    // vector 没有 push_front
    s.push_back(1);
}

int main() {
    std::vector<int> v;
    fill(v);
    std::list<int> l;
    fill(l);
}
