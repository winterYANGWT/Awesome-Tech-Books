// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// expect: counter=4 x=100 base=7 last=3
#include <stdio.h>

int main(void) {
    int counter = 0;   /* 共享：所有线程加同一个计数器 */
    int x = 100;       /* 共享变量，但在子句里被 private 覆盖 */
    int base = 7;      /* firstprivate：进入时拷贝原值 */
    int last = -1;     /* lastprivate：退出时写回顺序上最后一次迭代的值 */
    int seen_x[4] = {0};
    int seen_f[4] = {0};

#pragma omp parallel for num_threads(4) schedule(static, 1) \
    shared(counter, seen_x, seen_f) private(x) firstprivate(base) lastprivate(last)
    for (int i = 0; i < 4; ++i) {
        x = 1000 + i;      /* 每个线程有自己的 x，初值不可依赖 */
        seen_x[i] = x;
        seen_f[i] = base;  /* 每个线程都拿到进入时的 7 */
        last = i;
#pragma omp atomic
        counter += 1;
    }

    /* x 仍是 100，base 仍是 7：private 和 firstprivate 都不写回。 */
    printf("counter=%d x=%d base=%d last=%d\n", counter, x, base, last);
    printf("seen_x=%d %d %d %d\n", seen_x[0], seen_x[1], seen_x[2], seen_x[3]);
    printf("seen_f=%d %d %d %d\n", seen_f[0], seen_f[1], seen_f[2], seen_f[3]);
    return 0;
}
