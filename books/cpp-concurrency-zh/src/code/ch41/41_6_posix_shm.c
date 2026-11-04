// mode: run
// POSIX 共享内存：shm_open 得到一个 fd，对象有名字，ftruncate + mmap 之后才能用。
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    const char text[] = "hello from child via POSIX shm";
    char name[64];
    int fd;
    char *p;
    pid_t pid;

    snprintf(name, sizeof name, "/ch41_shm_%ld", (long)getpid());
    fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, 4096) == -1) {
        perror("ftruncate");
        return 1;
    }
    p = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd);

    pid = fork();
    if (pid == 0) {
        memcpy(p, text, sizeof text);
        _exit(0);
    }
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (waitpid(pid, NULL, 0) != pid) {
        perror("waitpid");
        return 1;
    }
    printf("parent read: %s\n", p);
    if (munmap(p, 4096) == -1) {
        perror("munmap");
        return 1;
    }
    if (shm_unlink(name) == -1) {
        perror("shm_unlink");
        return 1;
    }
    printf("shm_unlink: ok\n");
    return 0;
}
