// 第 16.6 节：函数实参的求值顺序未指定，先算进具名变量
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> evaluation_order;

int next(std::string name) {
    evaluation_order.push_back(name);
    return static_cast<int>(evaluation_order.size());
}

void report(int a, int b, const char* label) {
    std::cout << label << ": a=" << a << ", b=" << b << "；求值顺序:";
    for (const std::string& name : evaluation_order) {
        std::cout << ' ' << name;
    }
    std::cout << '\n';
    evaluation_order.clear();
}

int main() {
    // 两个 next 调用的先后由编译器决定，a/b 各拿到哪个值也随之改变
    report(next("first"), next("second"), "未指定");

    // 语句边界把顺序固定下来，读代码的人也不用猜
    int first = next("first");
    int second = next("second");
    report(first, second, "具名变量");
}
