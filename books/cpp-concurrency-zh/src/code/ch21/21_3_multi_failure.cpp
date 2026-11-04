// mode: run
// expect: 只 get 到第一个就退出：可见异常数 1 / 实际失败数 2\n逐个 get 并 catch：可见异常数 2 / 实际失败数 2
// 两个块各有一个坏值。第一种写法在第一个 get() 抛出后离开循环，第二个异常
// 留在 future 的共享状态里，没有人再取；第二种写法逐个 get() 并 catch。
#include <cstdio>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr int kFailures = 2;

class JoinThreads {
public:
    explicit JoinThreads(std::vector<std::thread>& threads) : threads_(threads) {}
    ~JoinThreads() {
        for (std::thread& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

private:
    std::vector<std::thread>& threads_;
};

int run(bool collect_all) {
    std::vector<int> data(100);
    for (int i = 0; i < 100; ++i) {
        data[static_cast<std::size_t>(i)] = i + 1;
    }

    std::vector<std::thread> threads;
    std::vector<std::future<long long>> futures;
    JoinThreads joiner(threads);

    for (std::size_t block = 0; block < 4; ++block) {
        const std::size_t begin = block * 25;
        const std::size_t end = begin + 25;
        std::packaged_task<long long()> task([&data, block, begin, end] {
            long long sum = 0;
            for (std::size_t i = begin; i < end; ++i) {
                if (data[i] == 7 || data[i] == 63) {
                    throw std::runtime_error("块 " + std::to_string(block) +
                                             " 遇到坏数据");
                }
                sum += data[i];
            }
            return sum;
        });
        futures.push_back(task.get_future());
        threads.emplace_back(std::move(task));
    }

    int visible = 0;
    if (collect_all) {
        for (std::future<long long>& f : futures) {
            try {
                (void)f.get();
            } catch (const std::exception&) {
                ++visible;
            }
        }
        std::printf("逐个 get 并 catch：可见异常数 %d / 实际失败数 %d\n", visible,
                    kFailures);
    } else {
        try {
            for (std::future<long long>& f : futures) {
                (void)f.get();
            }
        } catch (const std::exception&) {
            visible = 1;
        }
        std::printf("只 get 到第一个就退出：可见异常数 %d / 实际失败数 %d\n",
                    visible, kFailures);
    }
    return visible;
}

}  // namespace

int main() {
    (void)run(false);
    (void)run(true);
    return 0;
}
