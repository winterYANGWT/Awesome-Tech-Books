// mode: crash
// expect: terminate called after throwing an instance of 'std::runtime_error'
// 工作线程抛出的异常没有 catch，逃出线程函数边界；std::thread 的启动函数
// 不允许异常穿过，运行时直接调用 std::terminate。
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

void worker() {
    std::cout << "工作线程 " << std::this_thread::get_id() << " 开始" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    throw std::runtime_error("工作线程里没有 catch");
}

}  // namespace

int main() {
    std::cout << "主线程 " << std::this_thread::get_id() << " 创建工作线程" << std::endl;
    std::thread t(worker);
    t.join();
    std::cout << "不会走到这里\n";
    return 0;
}
