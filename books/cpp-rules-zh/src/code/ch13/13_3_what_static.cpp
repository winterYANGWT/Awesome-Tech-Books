// 第 13.3 节：用函数内 static 缓冲区保存消息，后一次 what() 会改掉前一次的结果
#include <cstdio>
#include <exception>
#include <iostream>
#include <string>

class JobError : public std::exception {
public:
    explicit JobError(std::string job) : job_(std::move(job)) {}

    const char* what() const noexcept override {
        // 函数返回后指针仍然有效，但所有 JobError 共享同一块缓冲区。
        static char buffer[128];
        std::snprintf(buffer, sizeof buffer, "job failed: %s", job_.c_str());
        return buffer;
    }

private:
    std::string job_;
};

int main() {
    const JobError disk("backup-disk");
    const JobError network("backup-network");

    const char* first = disk.what();
    std::cout << "第一次取到的消息：" << first << "\n";

    const char* second = network.what();
    std::cout << "第二次取到的消息：" << second << "\n";

    // first 没有悬垂，但它指向的缓冲区已经被第二次调用覆盖。
    std::cout << "再打印第一次的指针：" << first << "\n";
}
