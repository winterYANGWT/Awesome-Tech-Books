// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// expect: task_sum=36 serial_sum=36 tasks=15 leaves=8

#include <cstdio>
#include <omp.h>

namespace {
constexpr int kNodes = 15;
constexpr int kFirstLeaf = 7;
const int kValue[kNodes] = {0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

// 每个内部结点生成两个子任务；叶子把值加进共享的 sum。
void walk(int node, long long* sum, int* tasks, int* leaves) {
#pragma omp atomic
    *tasks += 1;

    if (node >= kFirstLeaf) {
#pragma omp atomic
        *sum += kValue[node];
#pragma omp atomic
        *leaves += 1;
        return;
    }

#pragma omp task shared(sum, tasks, leaves) firstprivate(node)
    walk(2 * node + 1, sum, tasks, leaves);
#pragma omp task shared(sum, tasks, leaves) firstprivate(node)
    walk(2 * node + 2, sum, tasks, leaves);
#pragma omp taskwait
}
}  // namespace

int main() {
    long long sum = 0;
    int tasks = 0;
    int leaves = 0;

#pragma omp parallel num_threads(4)
    {
#pragma omp single
        walk(0, &sum, &tasks, &leaves);
    }

    long long serial = 0;
    for (int i = kFirstLeaf; i < kNodes; ++i) serial += kValue[i];

    std::printf("task_sum=%lld serial_sum=%lld tasks=%d leaves=%d\n", sum, serial, tasks, leaves);
    return sum == serial ? 0 : 1;
}
