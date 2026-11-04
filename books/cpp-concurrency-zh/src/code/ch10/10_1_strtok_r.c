// mode: run
// expect: 运行次数: 1000\n至少一个线程出错的运行: 0
// 与 10_1_strtok_shared.c 相同，但每个线程带自己的 saveptr。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

enum { kWords = 100, kRuns = 1000 };

static pthread_barrier_t gate;

typedef struct {
    char text[512];
    char prefix;
    int bad_words;
    int bad_count;
} result_t;

// ANCHOR: worker
static void *worker(void *arg) {
    result_t *r = arg;
    char buf[512];
    char *save = NULL;
    strcpy(buf, r->text);

    pthread_barrier_wait(&gate);
    int n = 0;
    for (char *tok = strtok_r(buf, " ", &save); tok != NULL;
         tok = strtok_r(NULL, " ", &save)) {
        ++n;
        if (tok[0] != r->prefix || strlen(tok) != 4) ++r->bad_words;
        sched_yield();
    }
    if (n != kWords) ++r->bad_count;
    return NULL;
}
// ANCHOR_END: worker

static void fill(result_t *r, char prefix) {
    r->prefix = prefix;
    r->bad_words = 0;
    r->bad_count = 0;
    char *p = r->text;
    for (int i = 0; i < kWords; ++i) {
        p += snprintf(p, sizeof r->text - (size_t)(p - r->text), "%c%03d%s",
                      prefix, i, i + 1 == kWords ? "" : " ");
    }
}

int main(void) {
    if (pthread_barrier_init(&gate, NULL, 2) != 0) return 1;
    int bad_runs = 0;
    int bad_words = 0;
    int bad_counts = 0;
    for (int i = 0; i < kRuns; ++i) {
        result_t a;
        result_t b;
        fill(&a, 'a');
        fill(&b, 'b');
        pthread_t ta;
        pthread_t tb;
        if (pthread_create(&ta, NULL, worker, &a) != 0) return 1;
        if (pthread_create(&tb, NULL, worker, &b) != 0) return 1;
        pthread_join(ta, NULL);
        pthread_join(tb, NULL);
        if (a.bad_count || b.bad_count || a.bad_words || b.bad_words) ++bad_runs;
        bad_words += a.bad_words + b.bad_words;
        bad_counts += a.bad_count + b.bad_count;
    }
    pthread_barrier_destroy(&gate);
    printf("运行次数: %d\n", kRuns);
    printf("至少一个线程出错的运行: %d\n", bad_runs);
    printf("出现对方词的线程次数: %d\n", bad_words);
    printf("词数不对的线程次数: %d\n", bad_counts);
    return 0;
}
