// 第 33.1 节：从 child_bad.h 进入循环包含
#include "child_bad.h"

int main() {
    Child child;
    Parent parent;
    child.attach(&parent);
    parent.attach(&child);
}
