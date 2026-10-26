// 第 12.2 节：可能抛异常的计算不要和输出挤在同一条语句里
// mode: run
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>

int total(bool broken) {
    if (broken) {
        throw std::runtime_error("no readings");
    }
    return 42;
}

// 反例：计算是输出表达式的一部分，计算抛出时前缀已经写出去了
void write_together(std::ostream& out, bool broken) {
    out << "total: " << total(broken) << "\n";
}

// 正例：先把结果算出来，失败时输出流一个字符都没碰过
void write_after_compute(std::ostream& out, bool broken) {
    const int value = total(broken);
    out << "total: " << value << "\n";
}

int main() {
    std::ostringstream together;
    try {
        write_together(together, true);
    } catch (const std::exception& e) {
        std::printf("together failed: %s, wrote=[%s]\n", e.what(),
                    together.str().c_str());
    }

    std::ostringstream separated;
    try {
        write_after_compute(separated, true);
    } catch (const std::exception& e) {
        std::printf("separated failed: %s, wrote=[%s]\n", e.what(),
                    separated.str().c_str());
    }

    std::ostringstream ok;
    write_after_compute(ok, false);
    std::printf("success: wrote=[%s]\n", ok.str().c_str());
}
