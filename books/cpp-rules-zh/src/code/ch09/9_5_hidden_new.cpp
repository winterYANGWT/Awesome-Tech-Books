// 第 9.5 节：只声明带额外参数的类级 new，普通 new 会被名字隐藏
// mode: fail
// expect: no matching function
#include <cstddef>
#include <new>

class Widget {
public:
    static void* operator new(std::size_t size, int tag) {
        (void)tag;
        return ::operator new(size);
    }
    int id = 0;
};

int main() {
    Widget* p = new Widget;   // 找不到 operator new(std::size_t)
    delete p;
}
