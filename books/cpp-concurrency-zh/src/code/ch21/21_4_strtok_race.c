// mode: race
// expect: data race
// 和 strtok 一样，把“下次从哪继续”放在函数静态变量里。两个线程共用一个
// 分词器，谁后写 save_ptr 谁就覆盖对方的解析位置。
// 说明：libc 没有为 TSan 插桩，直接调 strtok 看不到报告；这里把同一份状态
// 放进示例，报告才能落到源码行。strtok(3) 自己标的是 MT-Unsafe race:strtok。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static char* save_ptr;
static char work_a[64];
static char work_b[64];
static pthread_barrier_t first_token_done;

// ANCHOR: tokenizer
static char* legacy_strtok(char* str, const char* delim) {
    if (str != NULL) {
        save_ptr = str;
    }
    if (save_ptr == NULL) {
        return NULL;
    }
    save_ptr += strspn(save_ptr, delim);
    if (*save_ptr == '\0') {
        save_ptr = NULL;
        return NULL;
    }
    char* token = save_ptr;
    save_ptr += strcspn(save_ptr, delim);
    if (*save_ptr != '\0') {
        *save_ptr = '\0';
        ++save_ptr;
    } else {
        save_ptr = NULL;
    }
    return token;
}
// ANCHOR_END: tokenizer

struct Job {
    const char* name;
    const char* text;
    char* work;
    int count;
};

// ANCHOR: worker
static void* tokenize(void* arg) {
    struct Job* job = arg;
    for (int round = 0; round < 1000; ++round) {
        strcpy(job->work, job->text);
        int first = 1;
        for (char* token = legacy_strtok(job->work, ","); token != NULL;
             token = legacy_strtok(NULL, ",")) {
            if (round == 0) {
                printf("%s:%s\n", job->name, token);
                ++job->count;
                if (first) {
                    first = 0;
                    // 两个线程都拿到第一个词后，save_ptr 只可能指向其中一条串。
                    pthread_barrier_wait(&first_token_done);
                }
            }
        }
    }
    return NULL;
}
// ANCHOR_END: worker

int main(void) {
    struct Job a = {"A", "a1,a2,a3,a4", work_a, 0};
    struct Job b = {"B", "b1,b2,b3,b4", work_b, 0};
    pthread_barrier_init(&first_token_done, NULL, 2);
    pthread_t ta;
    pthread_t tb;
    pthread_create(&ta, NULL, tokenize, &a);
    pthread_create(&tb, NULL, tokenize, &b);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    pthread_barrier_destroy(&first_token_done);
    printf("A 输出 %d 个词，B 输出 %d 个词（各应有 4 个）\n", a.count, b.count);
    return 0;
}
