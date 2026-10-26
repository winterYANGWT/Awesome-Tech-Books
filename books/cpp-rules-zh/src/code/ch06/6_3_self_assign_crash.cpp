// 第 6.3 节：先 delete 再 new 的赋值在自赋值时读取已释放的内存
// mode: crash
// expect: heap-use-after-free
#include <cstddef>
#include <vector>

class Bitmap {
public:
    explicit Bitmap(std::size_t bytes) : pixels(bytes, 0) {}
    std::vector<unsigned char> pixels;
};

class Widget {
public:
    explicit Widget(std::size_t bytes) : pb_(new Bitmap(bytes)) {}
    ~Widget() { delete pb_; }

    Widget(const Widget& rhs) : pb_(new Bitmap(*rhs.pb_)) {}

    Widget& operator=(const Widget& rhs) {
        delete pb_;                    // 先释放旧资源
        pb_ = new Bitmap(*rhs.pb_);    // rhs 就是 *this 时，这里读已释放的 Bitmap
        return *this;
    }

    void touch() { pb_->pixels[0] = 1; }

private:
    Bitmap* pb_;
};

int main() {
    Widget widget(4096);
    Widget* alias = &widget;
    widget = *alias;   // 同一个对象，走的是最普通不过的赋值
    widget.touch();
}
