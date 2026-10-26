// 第 33.1 节：两个头文件都只依赖对方的名字
#include "child.h"
#include "parent.h"

#include <iostream>

int main() {
    Parent parent;
    Child child;
    parent.attach(&child);
    child.attach(&parent);

    std::cout << "parent has child: " << (parent.child() == &child ? "yes" : "no") << '\n';
    std::cout << "child has parent: " << (child.parent() == &parent ? "yes" : "no") << '\n';
}
