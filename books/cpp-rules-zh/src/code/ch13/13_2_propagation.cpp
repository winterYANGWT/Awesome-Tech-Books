// 第 13.2 节：异常自动跨层传播；错误码要靠每一层手写检查
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct IoError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// 底层：读失败时抛异常。它不知道调用方会怎么处理，也不假装知道。
std::vector<int> read_samples(bool fail) {
    if (fail) {
        throw IoError("sensor log is unreadable");
    }
    return {10, 12, 11};
}

// 中间层只做转发。它没有足够信息处理 I/O 失败，所以不捕获、不包装。
std::vector<int> load_samples(bool fail) {
    return read_samples(fail);
}

double average(const std::vector<int>& samples) {
    if (samples.empty()) {
        throw std::invalid_argument("average: empty sample set");
    }
    int sum = 0;
    for (int value : samples) {
        sum += value;
    }
    return static_cast<double>(sum) / static_cast<double>(samples.size());
}

// 错误码版本：同样的读取，返回 int 状态，结果写进输出参数。
int read_samples_code(bool fail, std::vector<int>& out) {
    if (fail) {
        out.clear();
        return -1;
    }
    out = {10, 12, 11};
    return 0;
}

// 中间层漏检了 read_samples_code 的返回值。编译器不会提醒。
std::vector<int> load_samples_code(bool fail) {
    std::vector<int> data;
    read_samples_code(fail, data);
    return data;
}

int main() {
    std::cout << "--- 异常路径 ---\n";
    try {
        const double avg = average(load_samples(true));
        std::cout << "平均值：" << avg << "\n";
    } catch (const IoError& e) {
        std::cout << "main 捕获到：" << e.what()
                  << "（中间层没有写任何传播代码）\n";
    }

    std::cout << "--- 错误码路径 ---\n";
    const std::vector<int> data = load_samples_code(true);
    std::cout << "中间层拿到 " << data.size() << " 个样本";
    if (data.empty()) {
        std::cout << "，错误码被丢掉，下游只看到空数据，原始原因已经丢失\n";
    }
    std::cout << "--- 正常路径 ---\n";
    std::cout << "平均值：" << average(load_samples(false)) << "\n";
}
