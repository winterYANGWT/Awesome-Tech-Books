// 第 2.6 节：返回 const T 会挡住移动，C++11 之后应直接返回 T
#include <iostream>
#include <vector>

struct Tracer {
    static int copies;
    static int moves;

    Tracer() = default;
    Tracer(const Tracer&) { ++copies; }
    Tracer(Tracer&&) noexcept { ++moves; }
};

int Tracer::copies = 0;
int Tracer::moves = 0;

const Tracer make_const() { return Tracer{}; }
Tracer make_plain() { return Tracer{}; }

int main() {
    std::vector<Tracer> v;
    v.reserve(4);

    // const 纯右值不能绑定到 T&&，只能走 const T&，于是拷贝。
    v.push_back(make_const());
    std::cout << "after const: copies=" << Tracer::copies
              << " moves=" << Tracer::moves << '\n';

    // 非 const 纯右值可以绑定到 T&&，于是移动。
    v.push_back(make_plain());
    std::cout << "after plain: copies=" << Tracer::copies
              << " moves=" << Tracer::moves << '\n';
}
