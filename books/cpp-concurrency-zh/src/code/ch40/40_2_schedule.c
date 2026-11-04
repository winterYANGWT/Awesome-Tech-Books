// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// output: varies
// expect: static: 0 0 0 1 1 1 2 2 3 3
// expect: static,2: 0 0 1 1 2 2 3 3 0 0
// expect: guided:

#include <omp.h>
#include <stdio.h>

/* 每种调度跑一遍 10 次迭代的循环，记录每个迭代由哪个线程执行。 */
static void print_row(const char* name, int kind) {
    int owner[10];
    for (int i = 0; i < 10; ++i) owner[i] = -1;

    switch (kind) {
        case 0:
#pragma omp parallel for num_threads(4) schedule(static)
            for (int i = 0; i < 10; ++i) {
                owner[i] = omp_get_thread_num();
            }
            break;
        case 1:
            /* 旧写法常见的 n/thread_count，这里 10/4 取整为 2。 */
#pragma omp parallel for num_threads(4) schedule(static, 10 / 4)
            for (int i = 0; i < 10; ++i) {
                owner[i] = omp_get_thread_num();
            }
            break;
        case 2:
#pragma omp parallel for num_threads(4) schedule(guided)
            for (int i = 0; i < 10; ++i) {
                owner[i] = omp_get_thread_num();
            }
            break;
        default:
#pragma omp parallel for num_threads(4) schedule(dynamic)
            for (int i = 0; i < 10; ++i) {
                owner[i] = omp_get_thread_num();
            }
            break;
    }

    printf("%s", name);
    for (int i = 0; i < 10; ++i) printf(" %d", owner[i]);
    printf("\n");
}

int main(void) {
    omp_set_dynamic(0);
    printf("iteration: 0 1 2 3 4 5 6 7 8 9\n");
    print_row("static:", 0);
    print_row("static,2:", 1);
    print_row("guided:", 2);
    print_row("dynamic:", 3);
    return 0;
}
