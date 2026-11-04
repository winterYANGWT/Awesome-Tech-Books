// mode: run
// expect: call_once: constructed=2 same_object=1
#include <cstdio>
#include <mutex>
#include <thread>

int constructed = 0;

struct Widget {
    int id;
    Widget() : id(++constructed) {}
};

Widget* get_call_once() {
    static std::once_flag flag;
    static Widget* p = nullptr;
    std::call_once(flag, [] { p = new Widget(); });
    return p;
}

Widget& get_local_static() {
    static Widget w;
    return w;
}

int main() {
    Widget* a = nullptr;
    Widget* b = nullptr;
    Widget* c = nullptr;
    Widget* d = nullptr;
    std::thread t1([&] { a = get_call_once(); c = &get_local_static(); });
    std::thread t2([&] { b = get_call_once(); d = &get_local_static(); });
    t1.join();
    t2.join();
    std::printf("call_once: constructed=%d same_object=%d\n", constructed, a == b);
    std::printf("local static: same_object=%d\n", c == d);

    delete a;
}
