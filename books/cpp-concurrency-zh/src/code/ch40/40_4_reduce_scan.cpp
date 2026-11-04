// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）
// expect: accumulate=36 reduce=36
// expect: partial_sum=1 3 6 10 15 21 28 36
// expect: inclusive_scan=1 3 6 10 15 21 28 36
// expect: exclusive_scan_init100=100 101 103 106 110 115 121 128
// expect: inclusive_scan_init100=101 103 106 110 115 121 128 136

#include <cstdio>
#include <execution>
#include <functional>
#include <numeric>
#include <vector>

static void print_row(const char* name, const std::vector<int>& values) {
    std::printf("%s=", name);
    for (int value : values) std::printf("%d ", value);
    std::printf("\n");
}

int main() {
    const std::vector<int> input{1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<int> partial(input.size());
    std::vector<int> inclusive(input.size());
    std::vector<int> exclusive(input.size());
    std::vector<int> inclusive_init(input.size());

    const int sum_accumulate = std::accumulate(input.begin(), input.end(), 0);
    const int sum_reduce = std::reduce(std::execution::par, input.begin(), input.end(), 0);
    std::partial_sum(input.begin(), input.end(), partial.begin());
    std::inclusive_scan(std::execution::par, input.begin(), input.end(), inclusive.begin());

    // exclusive_scan 的 init 紧跟 result；inclusive_scan 的 init 在 binary_op 之后。
    std::exclusive_scan(std::execution::par, input.begin(), input.end(), exclusive.begin(), 100);
    std::inclusive_scan(std::execution::par, input.begin(), input.end(), inclusive_init.begin(),
                        std::plus<>(), 100);

    std::printf("accumulate=%d reduce=%d\n", sum_accumulate, sum_reduce);
    print_row("partial_sum", partial);
    print_row("inclusive_scan", inclusive);
    print_row("exclusive_scan_init100", exclusive);
    print_row("inclusive_scan_init100", inclusive_init);
}
