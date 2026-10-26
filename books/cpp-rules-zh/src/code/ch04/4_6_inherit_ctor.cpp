// 第 4.6 节：构造函数不会自动继承；using Base::Base 是显式引入
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Device {
public:
    Device() : id_(0) { std::cout << "Device()\n"; }
    explicit Device(int id) : id_(id) { std::cout << "Device(int)\n"; }

    int id() const { return id_; }

private:
    int id_;
};

// using 把 Device 的构造函数引入 Sensor；它不是“继承”
class Sensor : public Device {
public:
    using Device::Device;

    void set_label(std::string label) { label_ = std::move(label); }
    const std::string& label() const { return label_; }

private:
    std::string label_ = "unset";
};

// 不写 using 时，只能手动把实参转发给基类
class LegacySensor : public Device {
public:
    explicit LegacySensor(int id) : Device(id) {}
};

int main() {
    static_assert(std::is_constructible_v<Sensor, int>);
    static_assert(!std::is_convertible_v<int, Sensor>,
                  "explicit 属性跟着被引入的构造函数一起保留");

    Sensor sensor(42);   // 调用引入的 Device(int)，label_ 用自己的默认成员初始化器
    std::cout << "sensor id=" << sensor.id() << "，label=" << sensor.label() << "\n";

    Sensor copy = sensor;   // 拷贝构造仍由 Sensor 自己生成，会复制 label_
    copy.set_label("copy");
    std::cout << "copy id=" << copy.id() << "，label=" << copy.label() << "\n";

    Sensor defaulted;   // using 不抑制 Sensor 自己的默认构造
    std::cout << "default id=" << defaulted.id() << "\n";

    LegacySensor old(7);
    std::cout << "legacy id=" << old.id() << "\n";
}
