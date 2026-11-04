// mode: crash
// expect: terminate called
// 没有守卫：主线程抛出异常时 thread 仍 joinable，析构调用 terminate。
#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

void worker() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

}  // namespace

int main() {
    std::thread thread(worker);
    throw std::runtime_error("主线程直接抛出");
}
