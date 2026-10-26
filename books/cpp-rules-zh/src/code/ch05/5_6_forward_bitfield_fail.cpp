// 第 5.6 节：函数模板名和位域也不能直接完美转发
// mode: fail
#include <cstddef>
#include <cstdint>
#include <utility>

void target(std::size_t);
void target(int (*fn)(int));

template <typename T>
void fwd(T&& value) {
    target(std::forward<T>(value));
}

template <typename T>
T work_on_value(T value) { return value + 1; }

struct Header {
    std::uint32_t version : 4;
    std::uint32_t total_length : 16;
};

int main() {
    // 直接调用都能编译
    target(work_on_value<int>);
    Header header{};
    target(header.total_length);

    // 经过转发函数
    fwd(work_on_value);          // 函数模板名没有唯一类型
    fwd(header.total_length);    // 非 const 引用不能绑定位域
}
