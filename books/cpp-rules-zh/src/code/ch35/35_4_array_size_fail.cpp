// 第 35.4 节：形参已经是指针，std::size 拿不到原数组长度
// mode: fail
// expect: no matching function
#include <iterator>

int count(int values[10]) {
    return static_cast<int>(std::size(values));
}

int main() {
    int samples[10]{};
    return count(samples);
}
