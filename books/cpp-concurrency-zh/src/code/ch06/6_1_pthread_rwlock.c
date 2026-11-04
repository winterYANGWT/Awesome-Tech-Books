// mode: run
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define READERS 6
#define WRITERS 2
#define QUERIES_PER_READER 200000
#define INSERTS_PER_WRITER 1000

/* pthread 函数返回错误码，不设置 errno；每个调用都要自己检查。 */
#define CHECK(call)                                                          \
    do {                                                                     \
        int rc_ = (call);                                                    \
        if (rc_ != 0) {                                                      \
            fprintf(stderr, "%s failed: %d\n", #call, rc_);                  \
            exit(1);                                                         \
        }                                                                    \
    } while (0)

struct node {
    int value;
    struct node *next;
};

static struct node *g_head;
static pthread_rwlock_t g_lock;

static int member(int value) {
    CHECK(pthread_rwlock_rdlock(&g_lock));
    int found = 0;
    for (struct node *cur = g_head; cur != NULL; cur = cur->next) {
        if (cur->value == value) {
            found = 1;
            break;
        }
        if (cur->value > value) break;
    }
    CHECK(pthread_rwlock_unlock(&g_lock));
    return found;
}

static int insert(int value) {
    CHECK(pthread_rwlock_wrlock(&g_lock));
    struct node **link = &g_head;
    while (*link != NULL && (*link)->value < value) link = &(*link)->next;
    if (*link != NULL && (*link)->value == value) {
        CHECK(pthread_rwlock_unlock(&g_lock));
        return 0;
    }
    struct node *fresh = malloc(sizeof *fresh);
    if (fresh == NULL) {
        CHECK(pthread_rwlock_unlock(&g_lock));
        return -1;
    }
    fresh->value = value;
    fresh->next = *link;
    *link = fresh;
    CHECK(pthread_rwlock_unlock(&g_lock));
    return 1;
}

static long g_hits[READERS];

static void *reader_real(void *arg) {
    long id = (long)arg;
    long local = 0;
    for (int i = 0; i < QUERIES_PER_READER; ++i) {
        if (member(i % 1000)) ++local;
    }
    g_hits[id] = local;
    return NULL;
}

static long g_inserted[WRITERS];

static void *writer_real(void *arg) {
    long id = (long)arg;
    long ok = 0;
    const int first = 1000 + (int)id * INSERTS_PER_WRITER;
    for (int i = 0; i < INSERTS_PER_WRITER; ++i) {
        int rc = insert(first + i);
        if (rc < 0) {
            fprintf(stderr, "insert %d failed\n", first + i);
            exit(1);
        }
        ok += rc;
    }
    g_inserted[id] = ok;
    return NULL;
}

int main(void) {
    CHECK(pthread_rwlock_init(&g_lock, NULL));
    for (int i = 0; i < 1000; ++i) {
        if (insert(i) != 1) {
            fprintf(stderr, "initial insert %d failed\n", i);
            return 1;
        }
    }

    pthread_t readers[READERS];
    pthread_t writers[WRITERS];
    for (long i = 0; i < READERS; ++i) CHECK(pthread_create(&readers[i], NULL, reader_real, (void *)i));
    for (long i = 0; i < WRITERS; ++i) CHECK(pthread_create(&writers[i], NULL, writer_real, (void *)i));
    for (int i = 0; i < READERS; ++i) CHECK(pthread_join(readers[i], NULL));
    for (int i = 0; i < WRITERS; ++i) CHECK(pthread_join(writers[i], NULL));

    long hits = 0;
    long inserted = 0;
    for (int i = 0; i < READERS; ++i) hits += g_hits[i];
    for (int i = 0; i < WRITERS; ++i) inserted += g_inserted[i];

    CHECK(pthread_rwlock_rdlock(&g_lock));
    long length = 0;
    int sorted = 1;
    for (struct node *cur = g_head; cur != NULL; cur = cur->next) {
        ++length;
        if (cur->next != NULL && cur->value >= cur->next->value) sorted = 0;
    }
    CHECK(pthread_rwlock_unlock(&g_lock));

    printf("读线程 %d，写线程 %d\n", READERS, WRITERS);
    printf("查询 %d 次，命中 %ld 次\n", READERS * QUERIES_PER_READER, hits);
    printf("插入成功 %ld 个，链表长度 %ld，有序=%s\n", inserted, length, sorted ? "是" : "否");
    printf("末尾元素 2999 在表中：%s\n", member(2999) ? "是" : "否");

    CHECK(pthread_rwlock_destroy(&g_lock));
    struct node *cur = g_head;
    while (cur != NULL) {
        struct node *next = cur->next;
        free(cur);
        cur = next;
    }
    return 0;
}
