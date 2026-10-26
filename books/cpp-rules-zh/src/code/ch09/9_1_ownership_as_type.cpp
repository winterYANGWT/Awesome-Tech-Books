// 第 9.1 节：把“谁负责 delete”写进类型，而不是留给运行期的 bool
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class Sensor {
public:
    explicit Sensor(std::string name) : name_(std::move(name)) {
        ++live_;
        std::cout << "  构造 " << name_ << "（存活 " << live_ << "）\n";
    }
    ~Sensor() {
        --live_;
        std::cout << "  析构 " << name_ << "（存活 " << live_ << "）\n";
    }
    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;

    const std::string& name() const { return name_; }
    static int live() { return live_; }

private:
    std::string name_;
    static int live_;
};

int Sensor::live_ = 0;

// 拥有元素的容器：元素类型自己说明“我要 delete”。
class SensorBank {
public:
    // 返回引用只是方便调用者观察；所有权已经归 bank。
    Sensor& adopt(std::unique_ptr<Sensor> sensor) {
        std::cout << "  SensorBank 接管 " << sensor->name() << "\n";
        owned_.push_back(std::move(sensor));
        return *owned_.back();
    }

    // 不拥有元素的视图：只借一个引用，生命周期由调用者保证。
    void watch(Sensor& sensor) {
        views_.push_back(std::ref(sensor));
    }

    void report() const {
        std::cout << "  SensorBank 拥有 " << owned_.size()
                  << " 个，观察 " << views_.size() << " 个\n";
        for (const auto& view : views_) {
            std::cout << "  观察对象：" << view.get().name() << "\n";
        }
    }

private:
    std::vector<std::unique_ptr<Sensor>> owned_;
    std::vector<std::reference_wrapper<Sensor>> views_;
};

int main() {
    // 类型系统直接证明：unique_ptr 不可拷贝，只能移动。
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<Sensor>>);
    static_assert(std::is_move_constructible_v<std::unique_ptr<Sensor>>);
    static_assert(!std::is_copy_assignable_v<std::unique_ptr<Sensor>>);

    {
        SensorBank bank;

        auto outside = std::make_unique<Sensor>("码头传感器");
        std::cout << "交给 bank 之前，outside 是否为空："
                  << (outside ? "否" : "是") << "\n";
        bank.adopt(std::move(outside));
        std::cout << "交给 bank 之后，outside 是否为空："
                  << (outside ? "否" : "是") << "\n";

        bank.watch(bank.adopt(std::make_unique<Sensor>("仓库传感器")));

        bank.report();
        std::cout << "作用域结束前，存活传感器 " << Sensor::live() << " 个\n";
    }

    std::cout << "作用域结束后，存活传感器 " << Sensor::live() << " 个\n";
}
