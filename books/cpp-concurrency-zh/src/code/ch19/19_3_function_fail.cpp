// mode: fail
// expect: copy-constructible
// packaged_task 只能移动，std::function 要求目标可拷贝，所以这一行编译不过。
#include <functional>
#include <future>
#include <utility>

int main() {
    std::packaged_task<int()> task([] { return 42; });
    std::function<void()> f = std::move(task);
    (void)f;
    return 0;
}
