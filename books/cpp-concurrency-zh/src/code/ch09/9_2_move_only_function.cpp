// mode: run
// std: c++23
// expect: 任务从队列取出并执行: 是
// expect: 结果: 37
// C++23 的 move_only_function 可以装下 move-only 的 packaged_task。
#include <cstdio>
#include <deque>
#include <functional>
#include <future>
#include <thread>
#include <type_traits>
#include <utility>

int main() {
    static_assert(!std::is_copy_constructible_v<std::packaged_task<int(int)>>);
    static_assert(!std::is_copy_constructible_v<std::move_only_function<void()>>);

    std::packaged_task<int(int)> task([](int x) { return x * x + 1; });
    std::future<int> result = task.get_future();

    // 把 packaged_task 移进类型擦除的包装；std::function 在这里编译不过。
    std::move_only_function<void()> job =
        [t = std::move(task)]() mutable { t(6); };

    std::deque<std::move_only_function<void()>> queue;
    queue.push_back(std::move(job));

    std::thread worker([&queue] {
        std::move_only_function<void()> next = std::move(queue.front());
        queue.pop_front();
        next();
    });
    worker.join();

    std::printf("任务从队列取出并执行: 是\n");
    std::printf("结果: %d\n", result.get());
}
