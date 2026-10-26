// 第 33.1 节：两个头文件互相包含时，编译顺序会缺名字
#ifndef CH33_PARENT_BAD_H
#define CH33_PARENT_BAD_H

#include "child_bad.h"

class Parent {
public:
    void attach(Child* child) { child_ = child; }
private:
    Child* child_ = nullptr;
};

#endif
