// 第 25.2 节：set 的第三个模板参数是类型，不是函数
// mode: fail
// expect: error
#include <set>
#include <string>

bool text_less(const std::string* a, const std::string* b) {
    return *a < *b;
}

int main() {
    std::set<std::string*, text_less> words;  // text_less 是函数，不是类型
    std::string pear = "Pear";
    words.insert(&pear);
}
