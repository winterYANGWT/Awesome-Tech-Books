// 第 8.4 节：隐式转换把裸指针悄悄带出 RAII 对象的生命周期
// mode: crash
// expect: heap-use-after-free
#include <iostream>
#include <memory>

struct Face {
    int size;
    explicit Face(int s) : size(s) {}
};

class Font {
public:
    explicit Font(int size) : face_(std::make_unique<Face>(size)) {}

    // 隐式转换：任何需要 Face* 的地方都能直接把 Font 递过去。
    operator Face*() const { return face_.get(); }

private:
    std::unique_ptr<Face> face_;
};

Font make_font() {
    return Font(12);
}

int main() {
    // 临时 Font 在整条语句结束时析构，p 随即悬垂。
    Face* p = make_font();
    std::cout << "拿到隐式转换的指针，临时 Font 已经析构" << std::endl;
    std::cout << "读取悬垂指针：" << p->size << std::endl;
}
