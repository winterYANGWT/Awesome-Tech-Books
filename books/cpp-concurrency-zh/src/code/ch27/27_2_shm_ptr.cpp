// mode: crash
// expect: SIGSEGV
// 共享段里存着堆上 std::string 的地址；exec 之后地址空间已经换掉，
// 同一个进程按这个地址解引用，得到 SIGSEGV。共享内存里存的指针不能跨进程用。
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <string>

struct Record {
    uint64_t ptr;      // 堆上 std::string 字符数据的地址
    uint32_t len;
    uint32_t pad;
};

// 读端：exec 之后重新按名字打开同一块共享段，再按里面存的地址去读。
static int read_side(const char *name) {
    int fd = shm_open(name, O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }
    void *p = mmap(nullptr, sizeof(Record), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    shm_unlink(name);  // 名字先删掉，进程崩溃也不留垃圾

    Record rec;
    memcpy(&rec, p, sizeof rec);
    printf("reader: 共享段里的指针是 0x%llx，长度 %u\n",
           (unsigned long long)rec.ptr, rec.len);
    fflush(stdout);

    // 地址空间已经换掉：这个地址指向旧堆，现在没有意义。
    const char *s = reinterpret_cast<const char *>(rec.ptr);
    volatile char first = s[0];
    printf("reader: 读到了 %c\n", first);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--read") == 0) {
        return read_side(argv[2]);
    }

    char name[64];
    snprintf(name, sizeof name, "/kvd-ch27-ptr-%d", (int)getpid());
    shm_unlink(name);  // 清掉上次崩溃留下的同名对象

    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, (off_t)sizeof(Record)) < 0) {
        perror("ftruncate");
        return 1;
    }
    void *p = mmap(nullptr, sizeof(Record), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // 堆上的 string：对象和字符数据都在私有地址空间里。
    std::string *value = new std::string(64, 'A');
    Record rec{};
    rec.ptr = reinterpret_cast<uint64_t>(value->data());
    rec.len = static_cast<uint32_t>(value->size());
    memcpy(p, &rec, sizeof rec);
    printf("writer: 把堆地址 0x%llx 写进共享段，然后 exec 读端\n",
           (unsigned long long)rec.ptr);
    fflush(stdout);

    // 同一个进程 exec：代码、堆、栈全部换掉，旧指针随地址空间一起消失。
    execl("/proc/self/exe", "27_2_shm_ptr", "--read", name, (char *)nullptr);
    perror("execl");
    return 1;
}
