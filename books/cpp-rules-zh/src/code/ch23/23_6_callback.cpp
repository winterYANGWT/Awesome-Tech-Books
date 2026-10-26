// 第 23.6 节：按函数对象的准则写泛型回调包装器
#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class Device {
public:
    explicit Device(std::string name) : name_(std::move(name)) {}

    void start() { ++starts_; }
    void stop() { ++stops_; }

    int starts() const { return starts_; }
    int stops() const { return stops_; }

private:
    std::string name_;
    int starts_ = 0;
    int stops_ = 0;
};

// 小、单态、可拷贝赋值：存目标指针和成员函数指针，operator() 是 const
template <typename T>
class Callback {
public:
    using Action = void (T::*)();

    // explicit：不打算让 Device& 隐式变成 Callback
    explicit Callback(T& target, Action action) : target_(&target), action_(action) {}
    void operator()() const { (target_->*action_)(); }

private:
    T* target_;
    Action action_;
};

// 需要把不同 T 的回调放进同一个容器时，用一个公共基类提供多态
class CallbackBase {
public:
    virtual void operator()() const = 0;
    virtual ~CallbackBase() = default;
};

template <typename T>
class PolyCallback : public CallbackBase {
public:
    using Action = void (T::*)();

    PolyCallback(T& target, Action action) : target_(&target), action_(action) {}
    void operator()() const override { (target_->*action_)(); }

private:
    T* target_;
    Action action_;
};

// 替用户省掉重复写模板实参，像 std::make_pair 一样推导 T
template <typename T>
PolyCallback<T> make_callback(T& target, void (T::*action)()) {
    return PolyCallback<T>(target, action);
}

int main() {
    Device router("router");
    Device pump("pump");

    std::vector<std::unique_ptr<CallbackBase>> callbacks;
    callbacks.push_back(std::make_unique<PolyCallback<Device>>(router, &Device::start));
    callbacks.push_back(std::make_unique<PolyCallback<Device>>(pump, &Device::stop));
    for (const auto& callback : callbacks) {
        (*callback)();
    }
    std::printf("多态调用后：router start %d 次，pump stop %d 次\n",
                router.starts(), pump.stops());

    // 存指针而不是引用，编译器生成的拷贝赋值就能用
    static_assert(std::is_copy_assignable_v<Callback<Device>>);
    static_assert(!std::is_convertible_v<Device&, Callback<Device>>);

    Callback<Device> first(router, &Device::start);
    Callback<Device> second(router, &Device::stop);
    first = second;
    first();
    std::printf("重新绑定后：router start %d 次，stop %d 次\n",
                router.starts(), router.stops());

    auto inferred = make_callback(pump, &Device::start);
    inferred();
    std::printf("make_callback 之后：pump start %d 次\n", pump.starts());
}
