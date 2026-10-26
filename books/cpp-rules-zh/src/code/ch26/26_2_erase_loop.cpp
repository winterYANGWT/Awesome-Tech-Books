// 第 26.2 节：删除时接住 erase 的返回值；修改容器的循环里不要缓存 end()/size()
#include <cstdio>
#include <vector>

struct Job {
    int id;
    bool rejected;
};

void printJobs(const char* label, const std::vector<Job>& jobs) {
    std::printf("%s:", label);
    for (const Job& j : jobs) std::printf(" %d", j.id);
    std::puts("");
}

void printInts(const char* label, const std::vector<int>& v) {
    std::printf("%s:", label);
    for (int x : v) std::printf(" %d", x);
    std::puts("");
}

int main() {
    std::vector<Job> jobs{{101, false}, {102, true}, {103, false},
                          {104, true},  {105, false}};
    for (auto it = jobs.begin(); it != jobs.end();) {
        if (it->rejected) {
            std::printf("删除任务 %d\n", it->id);
            it = jobs.erase(it);  // 返回值指向被删元素的后继，循环里不再 ++
        } else {
            ++it;
        }
    }
    printJobs("剩余任务", jobs);

    // 每轮重新读 size()：新追加的元素也会被处理
    std::vector<int> grow{1};
    for (std::size_t i = 0; i != grow.size(); ++i) {
        if (grow[i] < 4) grow.push_back(grow[i] + 1);
    }
    printInts("每轮读 size()", grow);

    // 把 size() 缓存在循环外：只处理最初的 1 个元素就停
    std::vector<int> frozen{1};
    const std::size_t cached = frozen.size();
    for (std::size_t i = 0; i != cached; ++i) {
        if (frozen[i] < 4) frozen.push_back(frozen[i] + 1);
    }
    printInts("缓存 size()", frozen);
}
