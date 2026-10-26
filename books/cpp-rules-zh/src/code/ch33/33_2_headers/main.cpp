// 第 33.2 节：客户端只包含自己用到的两个头文件
#include "account.h"
#include "account.h"   // 第二次包含应由 account.h 自己的守卫挡掉
#include "report.h"

#include <iostream>

int main() {
    billing::Account account("ada");
    std::cout << account << '\n';
}
