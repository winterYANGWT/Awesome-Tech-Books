// mode: run
// expect: 任务参数: 7
// expect: 任务返回值: 50
// packaged_task 把可调用对象和 future 绑在一起，move 进线程执行。
#include <cstdio>
#include <future>
#include <thread>
#include <utility>

int main() {
    std::packaged_task<int(int)> task([](int x) { return x * x + 1; });
    std::future<int> result = task.get_future();

    // packaged_task 是 move-only，不能按值传给线程，必须 std::move。
    std::thread worker(std::move(task), 7);
    const int value = result.get();
    worker.join();

    std::printf("任务参数: 7\n");
    std::printf("任务返回值: %d\n", value);
}
