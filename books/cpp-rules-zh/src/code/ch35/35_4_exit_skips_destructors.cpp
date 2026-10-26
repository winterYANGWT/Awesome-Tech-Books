// 第 35.4 节：exit 不展开栈，自动对象不会被销毁
#include <cstdlib>
#include <iostream>

struct Trace {
    const char* name;
    explicit Trace(const char* n) : name(n) {
        std::cout << "construct " << name << '\n';
    }
    ~Trace() {
        std::cout << "destroy " << name << '\n';
    }
};

Trace global{"global"};

void run() {
    Trace local{"local"};
    std::cout << "calling exit\n";
    std::exit(0);
}

int main() {
    run();
    std::cout << "unreachable\n";
}
