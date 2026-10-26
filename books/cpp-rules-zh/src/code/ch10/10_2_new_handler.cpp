// 第 10.2 节：new-handler 的调用循环，以及类级 new-handler 的安装与恢复
// sanitize: none
#include <cstddef>
#include <iostream>
#include <limits>
#include <new>

struct OutOfMemory {};

int global_calls = 0;

void global_handler() {
    ++global_calls;
    std::cout << "  全局 new-handler 第 " << global_calls << " 次被调用\n";
    if (global_calls == 3) throw OutOfMemory{};   // 不再返回，跳出 operator new 的循环
}

int widget_calls = 0;
void widget_handler() { ++widget_calls; }

// EC-49 的 RAII 小工具：构造时安装，析构时恢复原来的全局 handler
class NewHandlerHolder {
public:
    explicit NewHandlerHolder(std::new_handler handler)
        : previous_(std::set_new_handler(handler)) {}
    ~NewHandlerHolder() { std::set_new_handler(previous_); }
    NewHandlerHolder(const NewHandlerHolder&) = delete;
    NewHandlerHolder& operator=(const NewHandlerHolder&) = delete;

private:
    std::new_handler previous_;
};

class Widget {
public:
    static std::new_handler set_new_handler(std::new_handler handler) {
        std::new_handler previous = current_;
        current_ = handler;
        return previous;
    }
    static void* operator new(std::size_t size) {
        NewHandlerHolder holder(current_);
        std::cout << "  Widget::operator new 期间，全局 handler 是 Widget 的："
                  << (std::get_new_handler() == current_ ? "是" : "否") << "\n";
        return ::operator new(size);
    }
    static void operator delete(void* p) noexcept { ::operator delete(p); }

private:
    inline static std::new_handler current_ = nullptr;
};

int main() {
    std::cout << "1) 全局 handler 的查询与替换\n";
    std::new_handler initial = std::get_new_handler();
    std::cout << "  初始 handler 为空：" << (initial == nullptr ? "是" : "否") << "\n";
    std::new_handler old = std::set_new_handler(global_handler);
    std::cout << "  set_new_handler 返回旧 handler：" << (old == initial ? "是" : "否") << "\n";

    std::cout << "2) 请求一块大到不可能满足的内存，观察调用循环\n";
    try {
        void* huge = ::operator new(std::numeric_limits<std::size_t>::max() / 2);
        ::operator delete(huge);
    } catch (const OutOfMemory&) {
        std::cout << "  捕获 OutOfMemory：handler 抛异常后 operator new 停止重试\n";
    }
    std::cout << "  全局 handler 调用次数：" << global_calls << "\n";
    std::set_new_handler(old);   // 恢复原来的 handler

    std::cout << "3) 类级 handler：Widget 的 operator new 临时替换全局 handler\n";
    std::new_handler widget_old = Widget::set_new_handler(widget_handler);
    std::cout << "  Widget 之前的 handler 为空：" << (widget_old == nullptr ? "是" : "否") << "\n";
    Widget* w = new Widget;
    delete w;
    std::cout << "  Widget handler 调用次数：" << widget_calls
              << "（分配成功，不会被调用）\n";
    std::cout << "  离开 Widget::operator new 后全局 handler 已恢复："
              << (std::get_new_handler() == old ? "是" : "否") << "\n";
    Widget::set_new_handler(nullptr);
}
