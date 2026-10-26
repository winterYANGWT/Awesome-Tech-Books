// 第 29.3 节：翻译单元 B，和 A 共享同一份 inline 实体
#include "counter.h"

int b_bump() { return bump() + twice(2); }
int* b_calls_address() { return &calls(); }
int* b_shared_address() { return &shared_value; }
int (*b_square_address())(int) { return &square; }
