// mode: crash
// flags: -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fsanitize=address
// expect: stack-buffer-overflow
// fd 等于 FD_SETSIZE 时，FD_SET 写到 fd_set 的 128 字节之外；ASan 抓到栈越界。
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/select.h>

int main(void) {
    fd_set set;
    FD_ZERO(&set);

    // 不做边界检查：glibc 的 FD_SET 直接按 fd / 8 定位字节。
    FD_SET(FD_SETSIZE, &set);

    // 让这次写入有可观察的结果，防止被优化掉。
    volatile int is_set = FD_ISSET(FD_SETSIZE, &set);
    printf("FD_ISSET(FD_SETSIZE) = %d\n", is_set);
    return 0;
}
