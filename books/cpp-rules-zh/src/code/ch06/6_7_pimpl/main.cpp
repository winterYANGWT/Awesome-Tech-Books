// 第 6.7 节的客户端：只看到 widget.h，看不到 Impl 和数据成员
#include "widget.h"

#include <iostream>
#include <utility>

int main() {
    Widget first("first");
    first.rename("renamed");

    Widget copy(first);             // 深拷贝 Impl
    Widget moved(std::move(copy));  // 只搬 unique_ptr

    std::cout << "first=" << first.name()
              << " copy=" << moved.name() << '\n';

    Widget assigned("old");
    assigned = first;               // 深拷贝赋值
    std::cout << "assigned=" << assigned.name() << '\n';

    std::cout << "sizeof(Widget)=" << sizeof(Widget) << '\n';
}
