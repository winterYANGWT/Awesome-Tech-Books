// 第 16.7 节：重载一元 & 会劫持取地址，泛型代码要用 std::addressof
#include <iostream>
#include <memory>
#include <type_traits>

class Buffer {
public:
    Buffer() : data_{1, 2, 3, 4} {}

    int* data() { return data_; }
    const int* data() const { return data_; }

    // 故意返回内部数组的地址：&buffer 的类型变成 int*，不再是 Buffer*
    int* operator&() { return data_; }

private:
    int data_[4];
};

template <class T>
T* real_address(T& object) {
    return std::addressof(object);   // 绕过重载的 operator&
}

int main() {
    Buffer buffer;

    int* first = &buffer;             // 调用 Buffer::operator&
    Buffer* real = std::addressof(buffer);

    std::cout << std::boolalpha;
    std::cout << "&buffer == buffer.data(): " << (first == buffer.data()) << '\n';
    std::cout << "addressof(buffer) == real_address(buffer): "
              << (real == real_address(buffer)) << '\n';

    static_assert(std::is_same_v<decltype(&buffer), int*>);
    static_assert(std::is_same_v<decltype(std::addressof(buffer)), Buffer*>);
}
