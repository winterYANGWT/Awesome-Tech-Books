// mode: crash
// sanitize: address
// flags: -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fsanitize=address
// expect: stack-buffer-overflow
// fd 达到 FD_SETSIZE（1024）时，FD_SET 仍按 fd/8 定位字节，写到了 fd_set 之外。
// 关掉 glibc 的 __fdelt_chk，否则它先 abort，看不到 ASan 的栈越界报告。
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int main(void) {
    int last = -1;
    // 一直打开 /dev/null，直到拿到编号 >= FD_SETSIZE 的 fd。
    while (last < FD_SETSIZE) {
        last = open("/dev/null", O_RDONLY);
        if (last < 0) {
            perror("open");
            return 1;
        }
    }
    printf("highest fd = %d, FD_SETSIZE = %d\n", last, FD_SETSIZE);

    fd_set set;
    FD_ZERO(&set);
    // fd_set 只有 128 字节；fd 1024 落在第 129 个字节上。
    FD_SET(FD_SETSIZE, &set);

    // 读一次让写入有可观察的结果，避免被优化掉。
    volatile int is_set = FD_ISSET(FD_SETSIZE, &set);
    printf("FD_ISSET(FD_SETSIZE) = %d\n", is_set);
    return 0;
}
