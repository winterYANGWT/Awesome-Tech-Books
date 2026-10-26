// 第 29.3 节：头文件里的 inline 函数、inline 变量和 constexpr 函数
// 第 29.3 节：inline 函数和 inline 变量在多个翻译单元里是同一个实体
#ifndef CH29_COUNTER_H
#define CH29_COUNTER_H

// 每个包含这个头文件的翻译单元都会看到同一份 inline 定义。
inline int& calls() {
    static int count = 0;   // 所有翻译单元共享这一个 static
    return count;
}

inline int bump() { return ++calls(); }

inline int square(int x) { return x * x; }

// C++17 的 inline 变量：头文件里的定义不会造成重复定义。
inline int shared_value = 7;

// constexpr 函数隐式 inline，多个翻译单元包含它也不会重复定义。
constexpr int twice(int x) { return x * 2; }

int a_bump();
int b_bump();
int* a_calls_address();
int* b_calls_address();
int* a_shared_address();
int* b_shared_address();
int (*a_square_address())(int);
int (*b_square_address())(int);

#endif
