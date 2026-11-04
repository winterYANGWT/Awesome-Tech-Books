// kvd_shm：父进程建监听 fd 和共享哈希表，fork 4 个 worker；worker 自己 accept，
// 表和每桶读写锁都在共享段里。worker 持锁被 kill -9 后，父进程重建整个池。
//
// 为了稳定演示“持锁时崩溃”，worker 处理第一条 SET 时把 pid 写进
// --crash-hold-file，然后持着桶写锁睡 30 秒；脚本看到文件后 kill -9。
// 这是测试钩子，正常运行时不给这个参数。
#include "net.hpp"
#include "protocol.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

namespace {

constexpr int kBuckets = 64;
constexpr int kEntries = 2048;
constexpr int kKeyMax = 256;
constexpr int kValueMax = 256;
constexpr int kMaxWorkers = 8;
constexpr std::uint32_t kNone = 0xffffffffu;

// ANCHOR: structs
struct Entry {
    std::uint32_t next;  // 桶内链的下标；空闲时是空闲链的下标
    std::uint32_t klen;
    std::uint32_t vlen;
    char key[kKeyMax];
    char value[kValueMax];
};

struct alignas(64) BucketLock {
    pthread_rwlock_t lock;
};

// 整个结构都在 mmap(MAP_SHARED | MAP_ANONYMOUS) 里：没有指针，只有下标。
struct Shared {
    BucketLock buckets[kBuckets];
    Entry entries[kEntries];
    std::uint32_t heads[kBuckets];  // 每个桶的链头下标
    std::uint32_t free_head;
    std::uint32_t used;
    pthread_mutex_t alloc_lock;
    std::uint64_t served[kMaxWorkers];  // 每个 worker 只写自己那一格
};
// ANCHOR_END: structs

std::uint32_t hash_key(std::string_view key) {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : key) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

void init_shared(Shared *s) {
    // ANCHOR: init_locks
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    // 每把读写锁都放在共享段里，并声明可以跨进程使用。
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    for (int i = 0; i < kBuckets; ++i) {
        pthread_rwlock_init(&s->buckets[i].lock, &attr);
        s->heads[i] = kNone;
    }
    pthread_rwlockattr_destroy(&attr);
    // ANCHOR_END: init_locks

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s->alloc_lock, &mattr);
    pthread_mutexattr_destroy(&mattr);

    s->free_head = 0;
    s->used = 0;
    for (int i = 0; i < kEntries; ++i) {
        s->entries[i].next = (i + 1 < kEntries) ? std::uint32_t(i + 1) : kNone;
        s->entries[i].klen = 0;
        s->entries[i].vlen = 0;
    }
    std::memset(s->served, 0, sizeof s->served);
}

Shared *new_shared() {
    void *p = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        std::exit(1);
    }
    Shared *s = static_cast<Shared *>(p);
    init_shared(s);
    return s;
}

std::uint64_t sum_served(const Shared *s) {
    std::uint64_t total = 0;
    for (int i = 0; i < kMaxWorkers; ++i) total += s->served[i];
    return total;
}

// 每个 worker 私有的一层：只持有指向共享段的指针。
class SharedStore {
public:
    SharedStore(Shared *s, int worker, const char *crash_file)
        : s_(s), worker_(worker), crash_file_(crash_file) {}

    Response apply(const Request &req) {
        switch (req.op) {
        case Request::Op::Get: {
            std::string out;
            if (!get(req.key, out)) return Response::not_found();
            return Response::value(std::move(out));
        }
        case Request::Op::Set:
            if (!set(req.key, req.value)) return Response::error("table full");
            return Response::ok();
        case Request::Op::Del:
            return del(req.key) ? Response::deleted() : Response::not_found();
        }
        return Response::error("bad op");
    }

private:
    static bool same_key(const Entry &e, std::string_view key) {
        return e.klen == key.size() && std::memcmp(e.key, key.data(), key.size()) == 0;
    }

    std::uint32_t find_locked(std::uint32_t head, std::string_view key) const {
        for (std::uint32_t i = head; i != kNone; i = s_->entries[i].next) {
            if (same_key(s_->entries[i], key)) return i;
        }
        return kNone;
    }

    std::uint32_t alloc_locked() {
        pthread_mutex_lock(&s_->alloc_lock);
        std::uint32_t idx = s_->free_head;
        if (idx != kNone) {
            s_->free_head = s_->entries[idx].next;
            s_->entries[idx].next = kNone;
            ++s_->used;
        }
        pthread_mutex_unlock(&s_->alloc_lock);
        return idx;
    }

    void free_locked(std::uint32_t idx) {
        pthread_mutex_lock(&s_->alloc_lock);
        s_->entries[idx].next = s_->free_head;
        s_->free_head = idx;
        s_->entries[idx].klen = 0;
        s_->entries[idx].vlen = 0;
        --s_->used;
        pthread_mutex_unlock(&s_->alloc_lock);
    }

    bool get(std::string_view key, std::string &out) {
        const std::uint32_t b = hash_key(key) % kBuckets;
        pthread_rwlock_rdlock(&s_->buckets[b].lock);
        std::uint32_t idx = find_locked(s_->heads[b], key);
        if (idx != kNone) out.assign(s_->entries[idx].value, s_->entries[idx].vlen);
        pthread_rwlock_unlock(&s_->buckets[b].lock);
        ++s_->served[worker_];
        return idx != kNone;
    }

    // ANCHOR: set
    // 返回 false 表示表满，新 key 没有写进去；调用方据此回 ERROR。
    bool set(std::string_view key, std::string_view value) {
        const std::uint32_t b = hash_key(key) % kBuckets;
        pthread_rwlock_wrlock(&s_->buckets[b].lock);

        // 只对 key "crash" 触发，避免正常 SET 流量误触测试钩子。
        if (crash_file_ != nullptr && !held_ && key == "crash") {
            held_ = true;  // 只演示一次：持着写锁，等脚本 kill -9
            int fd = open(crash_file_, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd >= 0) {
                char buf[32];
                int n = std::snprintf(buf, sizeof buf, "%d\n", (int)getpid());
                if (write(fd, buf, (size_t)n) != n) {
                    // 写不进去也没关系，脚本会超时。
                }
                close(fd);
            }
            sleep(30);
        }

        std::uint32_t idx = find_locked(s_->heads[b], key);
        if (idx == kNone) {
            idx = alloc_locked();
            if (idx == kNone) {
                pthread_rwlock_unlock(&s_->buckets[b].lock);
                return false;  // 表满：让 SET 回 ERROR，而不是假装成功
            }
            s_->entries[idx].klen = (std::uint32_t)key.size();
            std::memcpy(s_->entries[idx].key, key.data(), key.size());
            s_->entries[idx].next = s_->heads[b];
            s_->heads[b] = idx;
        }
        s_->entries[idx].vlen = (std::uint32_t)value.size();
        std::memcpy(s_->entries[idx].value, value.data(), value.size());
        pthread_rwlock_unlock(&s_->buckets[b].lock);
        ++s_->served[worker_];
        return true;
    }
    // ANCHOR_END: set

    bool del(std::string_view key) {
        const std::uint32_t b = hash_key(key) % kBuckets;
        pthread_rwlock_wrlock(&s_->buckets[b].lock);
        std::uint32_t prev = kNone;
        std::uint32_t idx = s_->heads[b];
        while (idx != kNone && !same_key(s_->entries[idx], key)) {
            prev = idx;
            idx = s_->entries[idx].next;
        }
        if (idx == kNone) {
            pthread_rwlock_unlock(&s_->buckets[b].lock);
            return false;
        }
        if (prev == kNone) {
            s_->heads[b] = s_->entries[idx].next;
        } else {
            s_->entries[prev].next = s_->entries[idx].next;
        }
        free_locked(idx);
        pthread_rwlock_unlock(&s_->buckets[b].lock);
        ++s_->served[worker_];
        return true;
    }

    Shared *s_;
    int worker_;
    const char *crash_file_;
    bool held_ = false;
};

void serve_connection(int cfd, SharedStore &store) {
    Parser parser;  // 解析缓冲是每个 worker 私有的，不放进共享段
    char buf[4096];
    for (;;) {
        ssize_t n = recv(cfd, buf, sizeof buf, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        if (n == 0) return;
        parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
        for (;;) {
            auto result = parser.next();
            if (!result) break;
            std::string out;
            if (auto *req = std::get_if<Request>(&*result)) {
                out = format_response(store.apply(*req));
            } else {
                out = format_response(std::get<ParseError>(*result));
            }
            try {
                if (!write_all(cfd, out)) return;
            } catch (...) {
                return;
            }
        }
    }
}

[[noreturn]] void worker_loop(int lfd, Shared *sh, int index, const char *crash_file) {
    SharedStore store(sh, index, crash_file);
    for (;;) {
        int cfd = accept(lfd, nullptr, nullptr);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            _exit(1);
        }
        serve_connection(cfd, store);
        close(cfd);
    }
}

struct Pool {
    pid_t pid[kMaxWorkers];
};

void fork_workers(int lfd, Shared *sh, int n, const char *crash_file, Pool &pool) {
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            std::exit(1);
        }
        if (pid == 0) {
            worker_loop(lfd, sh, i, crash_file);  // 不返回
        }
        pool.pid[i] = pid;
    }
}

// 回收所有已退出的子进程，返回回收的个数。每个 worker 号码对应一个槽。
int reap_children(Pool &pool, int n) {
    int count = 0;
    for (;;) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) break;
        ++count;
        for (int i = 0; i < n; ++i) {
            if (pool.pid[i] != pid) continue;
            pool.pid[i] = -1;
            if (WIFSIGNALED(status)) {
                std::printf("父进程: worker %d 被信号 %d 终止\n", (int)pid,
                            WTERMSIG(status));
            } else {
                std::printf("父进程: worker %d 退出，状态 %d\n", (int)pid,
                            WEXITSTATUS(status));
            }
        }
    }
    return count;
}

// 读写锁没有 robust 版本：持锁者死后锁不会自己释放，只能停掉整池、重建。
// ANCHOR: rebuild
void rebuild_pool(int lfd, Shared *&sh, int n, Pool &pool, std::uint64_t &before) {
    // 先停掉所有 worker 并回收，再读 served：否则它们仍可能在 ++served[worker_]，
    // 与这里的逐个读取构成数据竞争。
    for (int i = 0; i < n; ++i) {
        if (pool.pid[i] > 0) kill(pool.pid[i], SIGKILL);
    }
    while (waitpid(-1, nullptr, 0) > 0) {
    }
    before = sum_served(sh);
    std::printf("父进程: 已停掉其余 worker，恢复前已服务 %llu 个请求\n",
                (unsigned long long)before);
    munmap(sh, sizeof(Shared));
    sh = new_shared();  // 新的段、新的锁、空的表
    fork_workers(lfd, sh, n, nullptr, pool);
    std::printf("父进程: 新池已就绪，恢复前写入的数据已丢失\n");
}
// ANCHOR_END: rebuild

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    const char *crash_file = nullptr;
    int workers = 4;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--crash-hold-file") == 0 && i + 1 < argc) {
            crash_file = argv[++i];
        } else if (std::strcmp(argv[i], "--workers") == 0 && i + 1 < argc) {
            workers = std::atoi(argv[++i]);
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F) "
                             "[--workers N] [--crash-hold-file F]\n", argv[0]);
        return 2;
    }
    if (workers < 1) workers = 1;
    if (workers > kMaxWorkers) workers = kMaxWorkers;

    std::setvbuf(stdout, nullptr, _IOLBF, 0);  // 日志重定向到文件时也逐行刷新
    signal(SIGPIPE, SIG_IGN);  // 客户端断开时不要被 SIGPIPE 杀掉

    // 先挡住这三个信号，再 fork：worker 继承屏蔽字，父进程用 sigwaitinfo 统一处理。
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    if (sigprocmask(SIG_BLOCK, &set, nullptr) < 0) {
        perror("sigprocmask");
        return 1;
    }

    int lfd = -1;
    try {
        lfd = listen_tcp(port, port_file);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "listen: %s\n", e.what());
        return 1;
    }

    Shared *sh = new_shared();
    Pool pool{};
    fork_workers(lfd, sh, workers, crash_file, pool);
    std::printf("server: workers=%d buckets=%d entries=%d\n", workers, kBuckets,
                kEntries);

    std::uint64_t before = 0;
    for (;;) {
        siginfo_t info;
        int sig = sigwaitinfo(&set, &info);
        if (sig < 0) {
            if (errno == EINTR) continue;
            perror("sigwaitinfo");
            break;
        }
        if (sig == SIGTERM || sig == SIGINT) {
            std::printf("父进程: 收到信号 %d，关闭\n", sig);
            break;
        }
        if (sig == SIGCHLD) {
            // 回收后若确实少了 worker，就重建。已经回收过的 SIGCHLD
            // 只会让循环多转一圈，reap_children 返回 0。
            if (reap_children(pool, workers) > 0) {
                rebuild_pool(lfd, sh, workers, pool, before);
            }
        }
    }

    for (int i = 0; i < workers; ++i) {
        if (pool.pid[i] > 0) kill(pool.pid[i], SIGKILL);
    }
    while (waitpid(-1, nullptr, 0) > 0) {
    }
    std::printf("server: served_before_recovery=%llu served_after_recovery=%llu\n",
                (unsigned long long)before,
                (unsigned long long)sum_served(sh));
    munmap(sh, sizeof(Shared));
    close(lfd);
    return 0;
}
