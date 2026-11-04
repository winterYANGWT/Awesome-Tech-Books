// mode: run
// output: varies
// expect: std::cout：800 行，首尾不一致的行 [1-9]\d*\nosyncstream：800 行，首尾不一致的行 0
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <syncstream>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kThreads = 8;
constexpr int kLines = 100;

struct Counts {
    int total = 0;
    int broken = 0;
};

// 行格式：T3|0042|0123...|T3 END。行首和行尾的线程号必须一致。
Counts count_file(const char* path) {
    std::ifstream in(path);
    Counts c;
    std::string line;
    while (std::getline(in, line)) {
        ++c.total;
        if (line.size() < 10 || line[0] != 'T' || line[2] != '|') {
            ++c.broken;
            continue;
        }
        const char who = line[1];
        const std::string tail = std::string("|T") + who + " END";
        if (line.compare(line.size() - tail.size(), tail.size(), tail) != 0) {
            ++c.broken;
        }
    }
    return c;
}

void write_payload(std::ostream& out) {
    for (int k = 0; k < 40; ++k) out << "0123456789";
}

void cout_phase() {
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kLines; ++i) {
                std::cout << 'T' << t << '|' << i << '|';
                write_payload(std::cout);
                std::cout << "|T" << t << " END\n";
            }
        });
    }
    for (auto& t : threads) t.join();
    std::cout.flush();
}

// ANCHOR: osync_phase
void osync_phase() {
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            std::osyncstream out(std::cout);  // 每个线程一个，各自缓冲
            for (int i = 0; i < kLines; ++i) {
                out << 'T' << t << '|' << i << '|';
                write_payload(out);
                out << "|T" << t << " END\n";
                out.emit();  // 整行一次写出
            }
        });
    }
    for (auto& t : threads) t.join();
    std::cout.flush();
}
// ANCHOR_END: osync_phase

}  // namespace

int main() {
    char path[] = "/tmp/ch10_osync_XXXXXX";
    int tmp = mkstemp(path);
    if (tmp < 0) return 1;
    close(tmp);
    const int saved = dup(STDOUT_FILENO);

    // 把 stdout 临时换成文件，让程序自己数行首尾不一致的行。
    if (!std::freopen(path, "w", stdout)) return 1;
    cout_phase();
    std::fflush(stdout);
    const Counts plain = count_file(path);

    if (!std::freopen(path, "w", stdout)) return 1;
    osync_phase();
    std::fflush(stdout);
    const Counts sync = count_file(path);

    std::fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
    std::remove(path);
    std::printf("std::cout：%d 行，首尾不一致的行 %d\n", plain.total, plain.broken);
    std::printf("osyncstream：%d 行，首尾不一致的行 %d\n", sync.total, sync.broken);
}
