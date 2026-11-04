// mode: run
// tsan: no（TSan 与 glibc 的 C11 mtx_t 不兼容，实测 SEGV）
// expect: counter = 2000000
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>

#define THREADS 2
#define INCREMENTS 1000000

static mtx_t counter_lock;
static uint64_t counter = 0;

static int worker(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTS; ++i) {
        if (mtx_lock(&counter_lock) != thrd_success) {
            return 1;
        }
        ++counter;
        if (mtx_unlock(&counter_lock) != thrd_success) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
#ifdef __STDC_NO_THREADS__
    printf("__STDC_NO_THREADS__ = %d\n", __STDC_NO_THREADS__);
#else
    printf("__STDC_NO_THREADS__ 未定义\n");
#endif

    if (mtx_init(&counter_lock, mtx_plain) != thrd_success) {
        puts("mtx_init 失败");
        return 1;
    }
    thrd_t threads[THREADS];
    for (int i = 0; i < THREADS; ++i) {
        if (thrd_create(&threads[i], worker, NULL) != thrd_success) {
            puts("thrd_create 失败");
            return 1;
        }
    }
    for (int i = 0; i < THREADS; ++i) {
        int result = 1;
        if (thrd_join(threads[i], &result) != thrd_success || result != 0) {
            puts("thrd_join 失败");
            return 1;
        }
    }
    mtx_destroy(&counter_lock);
    printf("counter = %" PRIu64 "\n", counter);
    puts("C11 mtx_* 调用都返回 thrd_success");
    return 0;
}
