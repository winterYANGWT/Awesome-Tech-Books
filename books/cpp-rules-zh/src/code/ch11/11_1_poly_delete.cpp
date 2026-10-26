// 第 11.1 节：多态基类的析构函数该 public virtual 还是 protected 非 virtual
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

struct Sensor {
    Sensor() { std::cout << "  构造 Sensor 子对象\n"; }
    virtual ~Sensor() { std::cout << "  析构 Sensor 子对象\n"; }  // 允许 delete Sensor*，必须是虚的
    virtual std::string name() const = 0;
};

class Thermometer : public Sensor {
public:
    Thermometer() { std::cout << "  构造 Thermometer\n"; }
    ~Thermometer() override { std::cout << "  析构 Thermometer\n"; }
    std::string name() const override { return "thermometer"; }

private:
    std::vector<double> samples_{0.0, 1.0, 2.0};   // 派生类自己的资源
};

// 只作为策略混入的基类：不打算让人通过基类指针删除
struct NonCopyable {
protected:
    ~NonCopyable() = default;             // 派生类能析构，外界 delete 不了
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

struct Plain {
    ~Plain() { std::cout << "  析构 Plain\n"; }   // 非虚，单独使用没问题
};

struct Detailed : Plain {
    ~Detailed() { std::cout << "  析构 Detailed\n"; }
    std::string note = "kept alive";
};

class Reading : private NonCopyable {     // 私有继承：复用实现，不是 is-a
public:
    explicit Reading(int value) : value_(value) {}
    int value() const { return value_; }

private:
    int value_;
};

static_assert(std::has_virtual_destructor_v<Sensor>);
static_assert(!std::has_virtual_destructor_v<NonCopyable>);
static_assert(!std::is_destructible_v<NonCopyable>);   // protected 析构对外不可见

int main() {
    std::cout << "--- 通过 Sensor* 删除派生对象 ---\n";
    {
        std::unique_ptr<Sensor> sensor = std::make_unique<Thermometer>();
        std::cout << "  name = " << sensor->name() << "\n";
    }   // unique_ptr<Sensor> 的删除器对 Sensor* 调用 delete，虚析构接管

    std::cout << "--- shared_ptr 从派生类构造时保存了删除器 ---\n";
    {
        std::shared_ptr<Plain> plain = std::make_shared<Detailed>();
    }   // 即使 ~Plain 非虚，控制块里删除的是 Detailed

    std::cout << "--- 栈上的策略类对象正常析构 ---\n";
    {
        Reading reading(42);
        std::cout << "  value = " << reading.value() << "\n";
    }
}
