// 第 5.6 节：五类让完美转发失败或推错的实参
// mode: fail
#include <vector>

void target(const std::vector<int>&);
void target(int (*fn)(int));
void target_pointer(int*);

template <typename T>
void fwd(T&& value) {
    target(std::forward<T>(value));
}

template <typename T>
void fwd_pointer(T&& value) {
    target_pointer(std::forward<T>(value));
}

int process_value(int x) { return x * 2; }
int process_value(int x, int priority) { return x * priority; }


int main() {
    // 这些直接调用都能编译，说明下一层本身接受这些实参
    target({1, 2, 3});
    target(process_value);
    target_pointer(0);

    // 经过转发函数后，问题就出来了
    fwd({1, 2, 3});                 // 花括号是非推导语境
    fwd_pointer(0);                 // 0 推成 int，不是指针
    fwd(process_value);             // 重载函数名没有唯一类型
}
