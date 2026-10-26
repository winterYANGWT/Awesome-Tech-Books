// 第 18.2 节：私有继承用于覆盖虚函数；同样的效果也能用组合加嵌套类做到
#include <iostream>
#include <type_traits>

class Timer {
public:
    explicit Timer(int period) : period_(period) {}
    virtual ~Timer() = default;

    void tick() { onTick(); }
    int period() const { return period_; }

private:
    virtual void onTick() const = 0;
    int period_;
};

// 私有继承：Timer 的公开 tick 在 UsageWidget 里变成 private，
// 但 Widget 仍然可以覆盖私有的 onTick。
class UsageWidget : private Timer {
public:
    UsageWidget() : Timer(10) {}

    void runFor(int ticks) {
        for (int i = 0; i < ticks; ++i) {
            tick();
        }
    }

    int ticks() const { return ticks_; }

private:
    void onTick() const override { ++ticks_; }
    mutable int ticks_ = 0;
};

// 组合加嵌套类：Widget 本身不是 Timer，Timer 只存在于实现里。
class ComposedWidget {
private:
    class Hook : public Timer {
    public:
        explicit Hook(ComposedWidget* owner) : Timer(5), owner_(owner) {}

    private:
        void onTick() const override { ++owner_->ticks_; }
        ComposedWidget* owner_;
    };

public:
    ComposedWidget() : hook_(this) {}

    void runFor(int ticks) {
        for (int i = 0; i < ticks; ++i) {
            hook_.tick();
        }
    }

    int ticks() const { return ticks_; }

private:
    Hook hook_;
    mutable int ticks_ = 0;
};

int main() {
    UsageWidget widget;
    widget.runFor(10);
    std::cout << "私有继承：runFor(10) 后 ticks=" << widget.ticks() << '\n';
    std::cout << std::boolalpha
              << "UsageWidget is_base_of<Timer>="
              << std::is_base_of_v<Timer, UsageWidget>
              << "，可转换为 Timer*="
              << std::is_convertible_v<UsageWidget*, Timer*> << '\n';

    ComposedWidget composed;
    composed.runFor(7);
    std::cout << "组合 + 嵌套类：runFor(7) 后 ticks=" << composed.ticks() << '\n';
    std::cout << "ComposedWidget is_base_of<Timer>="
              << std::is_base_of_v<Timer, ComposedWidget> << '\n';

    static_assert(std::is_abstract_v<Timer>);
    static_assert(std::is_base_of_v<Timer, UsageWidget>);
    static_assert(!std::is_convertible_v<UsageWidget*, Timer*>);
    static_assert(!std::is_base_of_v<Timer, ComposedWidget>);
}
