// 第 26.6 节：reserve 只改 capacity 不创建元素；越界访问用 at 会抛异常
#include <cstdio>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<int> samples;
    samples.reserve(4);
    std::printf("reserve(4) 后 size=%zu capacity=%zu\n",
                samples.size(), samples.capacity());

    try {
        (void)samples.at(0);  // size 仍是 0，下标 0 不存在
        std::puts("不该走到这里");
    } catch (const std::out_of_range&) {
        std::puts("at(0) 抛出 std::out_of_range");
    }

    samples.push_back(11);
    samples.push_back(22);
    samples.push_back(33);
    std::printf("push_back 三次后 size=%zu capacity=%zu，at(1)=%d\n",
                samples.size(), samples.capacity(), samples.at(1));

    // resize 才会真正创建元素：多出来的位置值初始化为 0
    samples.resize(5);
    std::printf("resize(5) 后 size=%zu，内容:", samples.size());
    for (int x : samples) std::printf(" %d", x);
    std::puts("");
}
