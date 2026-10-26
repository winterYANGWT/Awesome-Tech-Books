// 第 5.4 节：const T&&、vector<T>&& 和类模板的 T&& 都不是通用引用
// mode: fail
#include <string>
#include <vector>

template <typename T>
void const_rref(const T&&) {}

template <typename T>
void vector_rref(std::vector<T>&&) {}

template <typename T>
struct Box {
    void store(T&&) {}   // T 在 Box 实例化时已经确定，不再推导
};

int main() {
    std::string text = "left value";
    const_rref(text);    // 不能把左值绑定到 const T&&

    std::vector<int> values{1, 2, 3};
    vector_rref(values); // 不能把左值绑定到 vector<T>&&

    Box<std::string> box;
    box.store(text);     // 不能把左值绑定到 std::string&&
}
