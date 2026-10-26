// 第 5.6 节：完美转发失败后的几种绕法
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <initializer_list>
#include <vector>

void target(const std::vector<int>& values);
void target(std::size_t value);
void target(int (*fn)(int));
void target_pointer(int* pointer);

template <typename T>
void fwd(T&& value) {
    target(std::forward<T>(value));
}

template <typename T>
void fwd_pointer(T&& value) {
    target_pointer(std::forward<T>(value));
}

void target(const std::vector<int>& values) {
    int sum = 0;
    for (int v : values) sum += v;
    std::cout << "target(vector)：和 = " << sum << "\n";
}

void target(std::size_t value) {
    std::cout << "target(size_t)：" << value << "\n";
}

void target(int (*fn)(int)) {
    std::cout << "target(函数指针)：" << fn(21) << "\n";
}

void target_pointer(int* pointer) {
    std::cout << "target(int*)："
              << (pointer == nullptr ? "空指针" : "非空指针") << "\n";
}

int process_value(int x) { return x * 2; }
int process_value(int x, int priority) { return x * priority; }

template <typename T>
T work_on_value(T value) {
    return value + 1;
}

struct Header {
    std::uint32_t version : 4;
    std::uint32_t ihl : 4;
    std::uint32_t dscp : 6;
    std::uint32_t ecn : 2;
    std::uint32_t total_length : 16;
};

struct Limits {
    // C++17 的 inline 变量：类内声明即定义，取地址也有实体
    inline static constexpr std::size_t MinVals = 28;
};

int main() {
    // 花括号：先造一个有名字的 initializer_list，再转发
    auto il = {1, 2, 3, 4};
    fwd(il);

    // 空指针：用 nullptr，不要用 0 或 NULL
    fwd_pointer(nullptr);

    // 重载函数名：先转成目标函数指针类型
    int (*one_arg)(int) = process_value;
    fwd(one_arg);

    // 函数模板名：先指定实例化
    int (*instantiated)(int) = work_on_value<int>;
    fwd(instantiated);

    // 位域：先拷贝到普通变量
    Header header{};
    header.total_length = 1500;
    auto length = static_cast<std::uint16_t>(header.total_length);
    fwd(length);

    // 只有声明的 static const 整型成员：C++17 起用 inline static constexpr
    fwd(Limits::MinVals);
}
