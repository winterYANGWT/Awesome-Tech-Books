// 第 12.2 节：异常安全靠析构函数的自动清理，不靠到处补 catch
// mode: run
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string>

// 用一个计数模拟"打开/关闭"的外部资源，避免演示程序真的泄漏
struct Gate {
    int open = 0;

    void acquire() noexcept { ++open; }
    void release() noexcept { --open; }
};

Gate gate;

// 反例：获取和释放分散在函数两端，异常从中间穿过去
void legacy_import(bool bad_record) {
    gate.acquire();
    if (bad_record) {
        throw std::runtime_error("bad record");
    }
    gate.release();
}

// 正例：资源的所有权交给局部对象，正常返回和栈展开都会释放
class Lease {
public:
    explicit Lease(Gate& g) : gate_(g) { gate_.acquire(); }
    ~Lease() { gate_.release(); }

    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;

private:
    Gate& gate_;
};

void raii_import(bool bad_record) {
    Lease lease(gate);
    if (bad_record) {
        throw std::runtime_error("bad record");
    }
}

// 只在"检测到错误、而且自己处理不了"的地方 throw
int parse_port(const std::string& text) {
    if (text.empty()) {
        throw std::invalid_argument("empty port");
    }

    int value = 0;
    for (char ch : text) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            throw std::invalid_argument("port is not a number: " + text);
        }
        value = value * 10 + (ch - '0');
        if (value > 65535) {
            throw std::out_of_range("port out of range: " + text);
        }
    }
    return value;
}

int main() {
    try {
        legacy_import(true);
    } catch (const std::exception& e) {
        std::printf("legacy failed: %s, still open=%d\n", e.what(), gate.open);
    }
    gate.release();  // 只为让演示程序回到干净状态，真实代码里没有这一步

    try {
        raii_import(true);
    } catch (const std::exception& e) {
        std::printf("raii failed: %s, still open=%d\n", e.what(), gate.open);
    }

    std::printf("port 8080 -> %d\n", parse_port("8080"));

    int port = 80;  // 出错时就地处理的默认值
    try {
        port = parse_port("http");
    } catch (const std::invalid_argument& e) {
        std::printf("fallback to %d: %s\n", port, e.what());
    }
}
