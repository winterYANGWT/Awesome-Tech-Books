// 第 30.3 节：promise 只能 set 一次；未 set 就销毁会让等待方拿到 broken_promise
#include <future>
#include <iostream>

int main() {
    std::promise<void> p;
    auto fut = p.get_future();
    p.set_value();
    fut.wait();

    try {
        p.set_value();                      // 第二次 set 同一个 promise
    } catch (const std::future_error& e) {
        std::cout << "第二次 set_value 是 promise_already_satisfied: "
                  << (e.code() == std::future_errc::promise_already_satisfied) << '\n';
    }

    std::future<void> orphan;
    {
        std::promise<void> q;               // q 没有 set_value 就被销毁
        orphan = q.get_future();
    }
    try {
        orphan.get();
    } catch (const std::future_error& e) {
        std::cout << "promise 先销毁，等待方拿到 broken_promise: "
                  << (e.code() == std::future_errc::broken_promise) << '\n';
    }
}
