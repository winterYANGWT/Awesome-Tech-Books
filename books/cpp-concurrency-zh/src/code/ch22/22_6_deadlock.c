// mode: hang
// flags: -fopenmp
// expect: worker|2 秒后仍未结束
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <omp.h>

static void pause_a_bit(void) {
    struct timespec ts = {0, 100 * 1000 * 1000}; /* 100 ms */
    nanosleep(&ts, NULL);
}

static void worker(int id, omp_lock_t *a, omp_lock_t *b) {
    if (id == 0) {
        omp_set_lock(a);
        pause_a_bit();       /* 让另一个线程先拿到 b */
        omp_set_lock(b);
    } else {
        omp_set_lock(b);
        pause_a_bit();
        omp_set_lock(a);
    }
    printf("worker %d got both locks\n", id);
    omp_unset_lock(a);
    omp_unset_lock(b);
}

int main(void) {
    omp_lock_t a, b;
    omp_init_lock(&a);
    omp_init_lock(&b);
#pragma omp parallel num_threads(2)
    {
        worker(omp_get_thread_num(), &a, &b);
    }
    omp_destroy_lock(&a);
    omp_destroy_lock(&b);
    return 0;
}
