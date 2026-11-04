// mode: run
// expect: 主线程在 get.*重新抛出：块 0 遇到坏数据 13\n所有线程已 join，进程没有 terminate
// packaged_task 把返回值或异常存进共享状态，future::get() 时重新抛出；
// join_threads 守卫保证正常返回和异常退出都会 join 已创建的线程。
#include <cstdio>
#include <future>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr int kBadValue = 13;

// ANCHOR: guard
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

    JoinThreads(const JoinThreads&) = delete;
    JoinThreads& operator=(const JoinThreads&) = delete;

private:
    std::vector<std::thread>& threads_;
};
// ANCHOR_END: guard

}  // namespace

int main() {
    std::vector<int> data(100);
    for (int i = 0; i < 100; ++i) {
        data[static_cast<std::size_t>(i)] = i + 1;
    }

    std::vector<std::thread> threads;
    std::vector<std::future<long long>> futures;

    // 内层作用域让 joiner 先于最后一行打印析构：析构返回时线程才真的 join 完。
    {
        JoinThreads joiner(threads);
        try {
            for (std::size_t block = 0; block < 4; ++block) {
                const std::size_t begin = block * 25;
                const std::size_t end = begin + 25;
                // ANCHOR: task
                std::packaged_task<long long()> task([&data, block, begin, end] {
                    long long sum = 0;
                    for (std::size_t i = begin; i < end; ++i) {
                        if (data[i] == kBadValue) {
                            throw std::runtime_error("块 " + std::to_string(block) +
                                                     " 遇到坏数据 13");
                        }
                        sum += data[i];
                    }
                    return sum;
                });
                futures.push_back(task.get_future());
                threads.emplace_back(std::move(task));
                // ANCHOR_END: task
            }

            long long total = 0;
            for (std::future<long long>& f : futures) {
                total += f.get();
            }
            std::printf("总和 %lld\n", total);
        } catch (const std::system_error& e) {
            std::printf("线程创建失败：%s\n", e.what());
        } catch (const std::exception& e) {
            std::printf("主线程在 get() 重新抛出：%s\n", e.what());
        }
    }

    std::printf("所有线程已 join，进程没有 terminate\n");
    return 0;
}
