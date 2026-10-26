// 第 33.1 节：循环的另一半
#ifndef CH33_CHILD_BAD_H
#define CH33_CHILD_BAD_H

#include "parent_bad.h"

class Child {
public:
    void attach(Parent* parent) { parent_ = parent; }
private:
    Parent* parent_ = nullptr;
};

#endif
