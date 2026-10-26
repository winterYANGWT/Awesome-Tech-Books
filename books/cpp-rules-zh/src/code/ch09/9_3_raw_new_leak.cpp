// 第 9.3 节：裸 new 先完成、后面的实参再抛异常，已分配的对象没人清理
// mode: crash
// expect: 检测到泄漏
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>

struct Job {
    explicit Job(int n) : id(n) { ++live; }
    ~Job() { --live; }
    int id;
    static int live;
};
int Job::live = 0;

Job* make_job(int id) {
    if (id == 2) {
        throw std::runtime_error("第二个任务创建失败");
    }
    return new Job(id);
}

void enqueue(std::initializer_list<Job*> jobs) {
    for (Job* job : jobs) {
        std::cout << "  入队 Job " << job->id << "\n";
    }
}

int main() {
    try {
        // 花括号列表保证从左到右求值：Job 1 先 new 成功，Job 2 再抛异常。
        enqueue({new Job(1), make_job(2)});
    } catch (const std::exception& e) {
        std::cout << "捕获：" << e.what() << "\n";
    }

    std::cout << "仍存活的 Job：" << Job::live
              << " 个（指针已经丢失，没人能 delete）\n";
    if (Job::live != 0) {
        std::cout << "检测到泄漏，abort\n";
        std::abort();
    }
}
