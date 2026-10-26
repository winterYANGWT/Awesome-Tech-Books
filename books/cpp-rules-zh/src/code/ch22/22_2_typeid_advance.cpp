// 第 22.2 节：用 typeid 做“编译期 if”的两个分支都要合法，list 迭代器编译失败
// mode: fail
// expect: no viable overloaded '+='
#include <iterator>
#include <list>
#include <typeinfo>

template <class It, class Dist>
void advance_by(It& it, Dist d) {
    if (typeid(typename std::iterator_traits<It>::iterator_category) ==
        typeid(std::random_access_iterator_tag)) {
        it += d;  // list 迭代器不支持 +=，但这条语句仍然必须通过编译
    } else {
        if (d >= 0)
            while (d--) ++it;
        else
            while (d++) --it;
    }
}

int main() {
    std::list<int> pending{1, 2, 3};
    auto it = pending.begin();
    advance_by(it, 1);
}
