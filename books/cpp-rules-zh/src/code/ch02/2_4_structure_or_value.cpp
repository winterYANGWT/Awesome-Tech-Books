// 第 2.4 节：explicit 看的是“决定结构”还是“决定值”
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Image {
public:
    // 只给行数：决定结构，不决定图像内容，应该显式。
    explicit Image(std::size_t rows) : rows_(rows), title_("<untitled>") {}

    // 给标题：实参成为值的一部分，可以保留隐式转换。
    Image(const char* title) : rows_(0), title_(title) {}

    const std::string& title() const { return title_; }
    std::size_t rows() const { return rows_; }

private:
    std::size_t rows_;
    std::string title_;
};

void show(const Image& image) {
    std::cout << image.title() << " (" << image.rows() << " rows)\n";
}

// C++20 的 explicit(bool)：条件显式，泛型包装里很好用。
template <class T>
class Tagged {
public:
    explicit(std::is_arithmetic_v<T>) Tagged(T value) : value_(std::move(value)) {}
    const T& value() const { return value_; }

private:
    T value_;
};

int main() {
    show("logo.png");   // const char* -> Image 是值转换，允许

    static_assert(!std::is_convertible_v<std::size_t, Image>);
    static_assert(std::is_convertible_v<const char*, Image>);

    std::cout << std::boolalpha;
    std::cout << "size_t -> Image: "
              << std::is_convertible_v<std::size_t, Image> << '\n';
    std::cout << "const char* -> Image: "
              << std::is_convertible_v<const char*, Image> << '\n';

    Tagged<std::string> tag = std::string("v1.2");   // explicit(false)
    std::cout << "Tagged<string> = " << tag.value() << '\n';

    static_assert(!std::is_convertible_v<int, Tagged<int>>);
    static_assert(std::is_convertible_v<std::string, Tagged<std::string>>);
}
