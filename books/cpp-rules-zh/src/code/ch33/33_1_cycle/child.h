// 第 33.1 节：另一半也用前置声明
#ifndef CH33_CHILD_H
#define CH33_CHILD_H

class Parent;

class Child {
public:
    void attach(Parent* parent) { parent_ = parent; }
    Parent* parent() const { return parent_; }
private:
    Parent* parent_ = nullptr;
};

#endif
