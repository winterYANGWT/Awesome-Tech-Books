// mode: fail
// expect: has no member named .then.
// 注意：std::future 到今天也没有 then；C++26 的写法是 sender 的 | then(...)。
#include <future>
#include <iostream>

int main() {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    p.set_value(41);
    // 旧教程里的链式写法，std::future 没有这个成员。
    auto g = f.then([](std::future<int> ready) {
        return ready.get() + 1;
    });
    std::cout << g.get() << '\n';
}
