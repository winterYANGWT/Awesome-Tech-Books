// mode: race
// expect: data race
// 两个线程对同一个非原子计数器自增；读-改-写没有同步，TSan 报数据竞争。
#include <cstdio>
#include <thread>

namespace {

long counter = 0;

void add_many() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;
    }
}

}  // namespace

int main() {
    std::thread a(add_many);
    std::thread b(add_many);
    a.join();
    b.join();
    std::printf("counter = %ld\n", counter);
    return 0;
}
