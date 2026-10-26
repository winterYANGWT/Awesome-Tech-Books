// 第 23.1 节：默认按值捕获在成员函数里捕获的是 this，对象销毁后闭包悬垂
// mode: crash
// expect: heap-use-after-free
#include <cstdio>
#include <functional>
#include <memory>
#include <vector>

using Filter = std::function<bool(int)>;

// 全局的过滤器表：闭包的生命周期和创建它的函数无关
std::vector<Filter> filters;

class Sensor {
public:
    explicit Sensor(int limit) : limit_(limit) {}

    void installFilter() {
        // 显式写 [this]：闭包里存的是指针，不是 limit_ 的副本。
        // 写成 [=] 也一样——编译器改写的正是 this->limit_（C++20 起这种隐式捕获被弃用）。
        filters.emplace_back([this](int value) { return value < limit_; });
    }

private:
    int limit_;
};

int main() {
    auto sensor = std::make_unique<Sensor>(10);
    sensor->installFilter();
    sensor.reset();  // Sensor 在这里销毁，filters 里的闭包留下悬垂的 this

    std::puts("Sensor 已销毁，现在调用它安装的过滤器：");
    std::printf("结果 = %d\n", filters.front()(3));
}
