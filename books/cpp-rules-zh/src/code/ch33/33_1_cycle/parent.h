// 第 33.1 节：用前置声明打断环
#ifndef CH33_PARENT_H
#define CH33_PARENT_H

class Child;   // 只给名字，不包含 child.h

class Parent {
public:
    void attach(Child* child) { child_ = child; }
    Child* child() const { return child_; }
private:
    Child* child_ = nullptr;
};

#endif
