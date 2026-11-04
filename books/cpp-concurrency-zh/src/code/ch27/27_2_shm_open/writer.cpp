// 共享段里的定长结构：只放字符数组和长度，不放 std::string、不放指针。
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <type_traits>

struct Entry {
    char key[32];
    uint32_t key_len;
    char value[96];
    uint32_t value_len;
};

static_assert(std::is_trivially_copyable_v<Entry>);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s /shm-name\n", argv[0]);
        return 2;
    }

    int fd = shm_open(argv[1], O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, (off_t)sizeof(Entry)) < 0) {
        perror("ftruncate");
        return 1;
    }
    void *p = mmap(nullptr, sizeof(Entry), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    Entry entry{};
    const char *key = "lang";
    const char *value = "C++20";
    memcpy(entry.key, key, strlen(key));
    entry.key_len = (uint32_t)strlen(key);
    memcpy(entry.value, value, strlen(value));
    entry.value_len = (uint32_t)strlen(value);
    memcpy(p, &entry, sizeof entry);

    printf("writer: sizeof(Entry)=%zu key_len 偏移=%zu value_len 偏移=%zu\n",
           sizeof(Entry), offsetof(Entry, key_len), offsetof(Entry, value_len));
    printf("writer: key=%s value=%s，写完就退出\n", key, value);
    fflush(stdout);

    munmap(p, sizeof(Entry));
    close(fd);  // 不 shm_unlink：名字还在，reader 才能打开
    return 0;
}
