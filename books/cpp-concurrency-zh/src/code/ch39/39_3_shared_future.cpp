// mode: run
// 两个消费者各持一份 shared_future，get() 返回同一个对象的引用。
#include <future>
#include <iostream>
#include <string>
#include <thread>

int main() {
    std::promise<std::string> p;
    auto shared = p.get_future().share();
    p.set_value("payload");

    auto s1 = shared; // 每个副本都能 get 一次或多次
    auto s2 = shared;
    const std::string* addr1 = nullptr;
    const std::string* addr2 = nullptr;
    std::string value1;
    std::string value2;

    std::thread t1([s1, &addr1, &value1] {
        const std::string& v = s1.get();
        addr1 = &v;
        value1 = v;
    });
    std::thread t2([s2, &addr2, &value2] {
        const std::string& v = s2.get();
        addr2 = &v;
        value2 = v;
    });
    t1.join();
    t2.join();

    std::cout << "consumer 1: " << value1 << '\n';
    std::cout << "consumer 2: " << value2 << '\n';
    std::cout << "same address: " << (addr1 == addr2 ? "yes" : "no") << '\n';
}
