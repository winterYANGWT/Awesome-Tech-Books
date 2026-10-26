// 第 3.4 节：static_cast 向下转型不查运行期类型，猜错就是未定义行为
// mode: crash
// expect: stack-buffer-overflow
#include <cstdio>

struct Base {
    virtual ~Base() = default;
};

struct Alpha : Base {
    int number = 1;
};

struct Beta : Base {
    double reading = 2.0;
    double extra = 3.0;
};

int main() {
    Alpha alpha;
    Base* base = &alpha;

    // 静态类型上合法，但 base 实际指向 Alpha，不是 Beta
    Beta* wrong = static_cast<Beta*>(base);
    std::printf("pointer is %s\n", wrong ? "non-null" : "null");
    wrong->extra = 3.5;
    std::printf("extra %f\n", wrong->extra);
}
