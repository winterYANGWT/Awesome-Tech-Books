// 第 23.1 节：[=] 隐式捕获 this 在 C++20 被弃用，编译器会指出来
// mode: warn
// expect: -Wdeprecated-this-capture
#include <cstdio>
#include <functional>
#include <vector>

using Filter = std::function<bool(int)>;

std::vector<Filter> filters;

class Sensor {
public:
    explicit Sensor(int limit) : limit_(limit) {}

    void installFilter() const {
        // [=] 没有复制 limit_，它复制的是 this；读 limit_ 等于读 this->limit_。
        filters.emplace_back([=](int value) { return value < limit_; });
    }

private:
    int limit_;
};

int main() {
    Sensor sensor(10);
    sensor.installFilter();
    std::printf("3 < 10：%d\n", filters.front()(3) ? 1 : 0);
}
