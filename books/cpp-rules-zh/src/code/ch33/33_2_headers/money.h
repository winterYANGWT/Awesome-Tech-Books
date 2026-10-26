// 第 33.2 节：这个头文件自带 <string>，可以单独编译
#ifndef CH33_MONEY_H
#define CH33_MONEY_H

#include <string>

namespace billing {
struct Money {
    long long cents = 0;
    std::string currency = "CNY";
};
}

#endif
