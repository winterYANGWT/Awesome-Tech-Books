/* 第 35.2 节：C 调用方按未改编的 probe 找符号，和 C++ 实现链接会失败 */
#include <stdio.h>

int probe(int);

int main(void) {
    printf("%d\n", probe(41));
    return 0;
}
