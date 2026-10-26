// 第 8.4 节：RAII 类通过 get()、operator-> 提供受控的裸资源访问
#include <iostream>
#include <memory>
#include <utility>

// 模拟 C API 里的字体对象；真实 API 只认 Face*。
struct Face {
    int size;
    explicit Face(int s) : size(s) {}
};

extern "C" void face_set_size(Face* face, int size) {
    face->size = size;
}

extern "C" int face_get_size(const Face* face) {
    return face->size;
}

class Font {
public:
    explicit Font(int size) : face_(std::make_unique<Face>(size)) {}

    // get() 只借出指针，所有权仍在 Font 手里。
    Face* get() const { return face_.get(); }

    // 让 Font 用起来像指针，但生命周期仍由 Font 决定。
    Face* operator->() const { return face_.get(); }
    Face& operator*() const { return *face_; }

private:
    std::unique_ptr<Face> face_;
};

int main() {
    Font title(12);
    std::cout << "初始大小 = " << title->size << "\n";

    // 显式 get()：调用点能看出这里把裸指针交给了 C API。
    face_set_size(title.get(), 18);
    std::cout << "C API 修改后 = " << face_get_size(title.get()) << "\n";

    // operator-> 和 operator* 是另外两种受控访问。
    title->size += 2;
    std::cout << "通过 operator-> 后 = " << (*title).size << "\n";

    Face* borrowed = title.get();
    std::cout << "borrowed 与 title.get() 相同 = " << std::boolalpha
              << (borrowed == title.get()) << "\n";
    std::cout << "Font 仍拥有这块内存，borrowed 不能 delete\n";
}
