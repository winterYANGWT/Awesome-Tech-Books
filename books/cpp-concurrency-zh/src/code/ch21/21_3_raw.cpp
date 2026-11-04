// mode: crash
// expect: terminate called after throwing an instance of 'std::runtime_error'
// 裸线程版并行求和：工作线程里没有 catch，异常逃出线程函数边界。
// 主线程还在 join，进程已经被 terminate。
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

constexpr int kBadValue = 13;

long long sum_block(const std::vector<int>& data, std::size_t begin,
                    std::size_t end) {
    long long sum = 0;
    for (std::size_t i = begin; i < end; ++i) {
        if (data[i] == kBadValue) {
            throw std::runtime_error("块里遇到坏数据 13");
        }
        sum += data[i];
    }
    return sum;
}

}  // namespace

int main() {
    std::vector<int> data(100);
    for (int i = 0; i < 100; ++i) {
        data[static_cast<std::size_t>(i)] = i + 1;
    }

    std::vector<long long> results(4, 0);
    std::vector<std::thread> threads;
    std::printf("主线程启动 4 个工作线程\n");
    std::fflush(stdout);

    for (std::size_t block = 0; block < 4; ++block) {
        const std::size_t begin = block * 25;
        const std::size_t end = begin + 25;
        threads.emplace_back([&data, &results, block, begin, end] {
            results[block] = sum_block(data, begin, end);
        });
    }

    for (std::thread& t : threads) {
        t.join();
    }
    std::printf("不会走到这里\n");
    return 0;
}
