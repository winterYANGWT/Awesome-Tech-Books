// mode: run
// unique_ptr 只能移动，所有权随参数移进线程存储。
#include <cstdio>
#include <memory>
#include <string>
#include <thread>

int main() {
    auto text = std::make_unique<std::string>("所有权跟着线程走");
    std::printf("移动前主线程指针非空：%s\n", text != nullptr ? "是" : "否");

    std::thread worker([value = std::move(text)]() mutable {
        std::printf("线程收到：%s\n", value->c_str());
    });
    worker.join();

    std::printf("join 后主线程指针为空：%s\n", text == nullptr ? "是" : "否");
    return 0;
}
