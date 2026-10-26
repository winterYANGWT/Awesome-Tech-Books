// 第 6.7 节：Impl 不完整时在头文件里默认析构，unique_ptr 的删除器无法通过
// mode: fail
// expect: incomplete
#include <memory>

class Widget {
public:
    Widget() : pImpl(nullptr) {}
    ~Widget() = default;   // 隐式 inline，析构点看不到 Impl 的定义

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

int main() {
    Widget widget;   // 在这里实例化 Widget 的析构
}
