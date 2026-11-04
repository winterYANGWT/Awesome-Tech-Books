// mode: run
// output: varies
// tsan: no（单线程计时示例）
// timeout: 60
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace {

volatile unsigned long long sink;
constexpr int lines = 1'000'000;

double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

double copy_each_line(const std::string& buf) {
    const auto t0 = std::chrono::steady_clock::now();
    unsigned long long total = 0;
    std::size_t pos = 0;
    while (pos < buf.size()) {
        std::size_t end = buf.find('\n', pos);
        if (end == std::string::npos) end = buf.size();
        std::string line(buf.data() + pos, end - pos);
        total += line.size();
        total += static_cast<unsigned char>(line[0]);
        pos = end + 1;
    }
    sink = total;
    return ms_since(t0);
}

double view_each_line(const std::string& buf) {
    const auto t0 = std::chrono::steady_clock::now();
    unsigned long long total = 0;
    std::size_t pos = 0;
    while (pos < buf.size()) {
        std::size_t end = buf.find('\n', pos);
        if (end == std::string::npos) end = buf.size();
        std::string_view line(buf.data() + pos, end - pos);
        total += line.size();
        total += static_cast<unsigned char>(line[0]);
        pos = end + 1;
    }
    sink = total;
    return ms_since(t0);
}

}  // namespace

int main() {
    std::string buf;
    buf.reserve(static_cast<std::size_t>(lines) * 48);
    for (int i = 0; i < lines; ++i) {
        buf += "key";
        buf += std::to_string(i % 1000);
        buf += "=payload-0123456789abcdef\n";
    }

    double best_copy = 1e300;
    double best_view = 1e300;
    for (int r = 0; r < 3; ++r) {
        const double c = copy_each_line(buf);
        const double v = view_each_line(buf);
        if (c < best_copy) best_copy = c;
        if (v < best_view) best_view = v;
    }
    std::puts("method,ms");
    std::printf("copy_to_string,%.3f\n", best_copy);
    std::printf("string_view,%.3f\n", best_view);
}
