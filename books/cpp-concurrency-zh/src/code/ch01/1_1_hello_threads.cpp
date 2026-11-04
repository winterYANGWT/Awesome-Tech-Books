// mode: run
// expect: 7 行都到齐了
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// 两个执行流各写各的 vector：先不引入互斥量，只看 join 的作用。
void worker(std::vector<std::string>& lines) {
    for (int i = 0; i < 4; ++i) {
        lines.push_back("worker：第 " + std::to_string(i + 1) + " 行");
    }
}

int main() {
    std::vector<std::string> worker_lines;
    std::thread t(worker, std::ref(worker_lines));

    // 新线程干活的同时，主线程也完成自己那份。
    std::vector<std::string> main_lines;
    for (int i = 0; i < 3; ++i) {
        main_lines.push_back("main：第 " + std::to_string(i + 1) + " 行");
    }

    // 不 join 就读 worker_lines 是数据竞争；join 建立先行发生关系。
    t.join();

    for (const auto& line : main_lines) std::cout << line << '\n';
    for (const auto& line : worker_lines) std::cout << line << '\n';
    std::cout << "main：join 返回，" << main_lines.size() + worker_lines.size()
              << " 行都到齐了\n";
}
