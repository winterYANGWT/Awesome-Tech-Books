// mode: fail
// expect: request for member
// 解析歧义：这一行不是构造线程对象，而是声明了一个函数。
#include <thread>

struct background_task {
    void operator()() const {}
};

int main() {
    std::thread t(background_task());  // 看起来像构造，其实是函数声明
    t.join();                          // 因此 t 不是 std::thread 对象
    return 0;
}
