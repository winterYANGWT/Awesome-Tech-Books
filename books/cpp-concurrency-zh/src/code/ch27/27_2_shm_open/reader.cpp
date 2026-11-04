// reader 是另一个进程：writer 已经退出，共享段仍然按名字存在。
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>

struct Entry {
    char key[32];
    uint32_t key_len;
    char value[96];
    uint32_t value_len;
};

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s /shm-name\n", argv[0]);
        return 2;
    }

    int fd = shm_open(argv[1], O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }
    void *p = mmap(nullptr, sizeof(Entry), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    Entry entry;
    memcpy(&entry, p, sizeof entry);
    printf("reader: key=%.*s value=%.*s\n", (int)entry.key_len, entry.key,
           (int)entry.value_len, entry.value);
    fflush(stdout);

    shm_unlink(argv[1]);  // 最后一个使用者负责删名字
    munmap(p, sizeof(Entry));
    close(fd);
    return 0;
}
