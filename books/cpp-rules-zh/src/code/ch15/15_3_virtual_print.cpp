// 第 15.3 节：要不要虚输出，取决于是否需要为派生类保留多态
#include <iostream>
#include <ostream>
#include <string>
#include <utility>

namespace devices {

class Device {
public:
    virtual ~Device() = default;
    virtual void print(std::ostream& os) const { os << "Device"; }
};

class Camera : public Device {
public:
    explicit Camera(std::string id) : id_(std::move(id)) {}
    void print(std::ostream& os) const override { os << "Camera(" << id_ << ")"; }

private:
    std::string id_;
};

// 非虚自由函数：它本身不做分派，把工作交给虚函数 print。
std::ostream& operator<<(std::ostream& os, const Device& device) {
    device.print(os);
    return os;
}

// 对照：label() 不是虚函数，自由函数直接调用它。
class Report {
public:
    virtual ~Report() = default;
    std::string label() const { return "Report"; }
};

class AuditReport : public Report {
public:
    std::string label() const { return "AuditReport"; }
};

std::ostream& operator<<(std::ostream& os, const Report& report) {
    return os << report.label();
}

}  // namespace devices

int main() {
    devices::Camera camera("cam-1");
    const devices::Device& device = camera;
    std::cout << "Device&:      " << device << '\n';
    std::cout << "Camera:       " << camera << '\n';

    devices::AuditReport audit;
    const devices::Report& report = audit;
    std::cout << "Report&:      " << report << '\n';
    std::cout << "audit.label(): " << audit.label() << '\n';
}
