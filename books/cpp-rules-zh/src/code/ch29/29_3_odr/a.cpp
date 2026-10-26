// 第 29.3 节：翻译单元 A，调用头文件里的 inline 函数
#include "counter.h"

int a_bump() { return bump() + twice(1); }
int* a_calls_address() { return &calls(); }
int* a_shared_address() { return &shared_value; }
int (*a_square_address())(int) { return &square; }
