// RLIMIT_NOFILE 的软限制和硬限制：软限制可以自己往上抬到硬限制，硬限制要特权。
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

static void report(const char *what, int rc) {
    if (rc == 0) {
        printf("%s: ok\n", what);
    } else {
        printf("%s: %s\n", what, strerror(errno));
    }
}

int main(void) {
    struct rlimit orig;
    if (getrlimit(RLIMIT_NOFILE, &orig) < 0) {
        perror("getrlimit");
        return 1;
    }
    printf("RLIMIT_NOFILE soft=%llu hard=%llu\n",
           (unsigned long long)orig.rlim_cur,
           (unsigned long long)orig.rlim_max);
    printf("soft < hard: %s\n", orig.rlim_cur < orig.rlim_max ? "yes" : "no");

    struct rlimit r;
    if (orig.rlim_cur < orig.rlim_max) {
        r = orig;
        r.rlim_cur = orig.rlim_cur + 1;
        report("把软限制提高 1", setrlimit(RLIMIT_NOFILE, &r));
    } else {
        printf("把软限制提高 1: 软限制已等于硬限制，要先提高硬限制\n");
    }

    r = orig;
    r.rlim_max = orig.rlim_max + 1;
    if (r.rlim_cur > r.rlim_max) {
        r.rlim_cur = r.rlim_max;
    }
    int rc = setrlimit(RLIMIT_NOFILE, &r);
    report("把硬限制提高 1", rc);
    if (rc == 0) {
        // 有 CAP_SYS_RESOURCE 时刚才那一步会成功，恢复原值免得影响后面的判断。
        if (setrlimit(RLIMIT_NOFILE, &orig) < 0) {
            perror("restore");
        }
    }

    struct rlimit lower = orig;
    if (lower.rlim_max > 0) {
        lower.rlim_max -= 1;
    }
    if (lower.rlim_cur > lower.rlim_max) {
        lower.rlim_cur = lower.rlim_max;
    }
    report("把硬限制降低 1", setrlimit(RLIMIT_NOFILE, &lower));
    return 0;
}
