// 第 6.7 节：Impl 完整定义之后，再写特殊成员函数的定义
#include "widget.h"

#include <utility>
#include <vector>

struct Widget::Impl {
    std::string name;
    std::vector<int> data;
};

Widget::Widget() : pImpl(std::make_unique<Impl>()) {}

Widget::Widget(std::string name) : pImpl(std::make_unique<Impl>()) {
    pImpl->name = std::move(name);
}

// 定义点能看到完整的 Impl，unique_ptr 的默认删除器才能通过 static_assert。
Widget::~Widget() = default;

Widget::Widget(const Widget& rhs)
    : pImpl(std::make_unique<Impl>(*rhs.pImpl)) {}

Widget& Widget::operator=(const Widget& rhs) {
    // 先深拷贝到新的 Impl，成功后再替换旧指针；自赋值天然安全。
    auto copy = std::make_unique<Impl>(*rhs.pImpl);
    pImpl = std::move(copy);
    return *this;
}

Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;

void Widget::rename(std::string name) {
    pImpl->name = std::move(name);
}

const std::string& Widget::name() const {
    return pImpl->name;
}

std::size_t Widget::size() const {
    return pImpl->data.size();
}
