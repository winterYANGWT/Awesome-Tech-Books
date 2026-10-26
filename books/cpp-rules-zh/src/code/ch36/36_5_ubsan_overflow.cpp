// 第 36.5 节：UBSan 抓住有符号整数溢出，ASan 不管这类错误
// mode: crash
// sanitize: undefined
// expect: signed integer overflow
#include <iostream>

int next_balance() {
    return 500'000'000;
}

int main() {
    int balance = 2'000'000'000;
    balance += next_balance();  // 超过 INT_MAX，未定义行为
    std::cout << "balance=" << balance << '\n';
}
