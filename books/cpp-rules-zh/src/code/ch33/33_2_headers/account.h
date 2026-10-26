// 第 33.2 节：Account 按值含有 Money 和 string，必须看到两者的完整定义
#ifndef CH33_ACCOUNT_H
#define CH33_ACCOUNT_H

#include "money.h"
#include <string>

namespace billing {
class Account {
public:
    explicit Account(std::string owner);
    const std::string& owner() const;
    Money balance() const;

private:
    std::string owner_;
    Money balance_{1250, "CNY"};
};
}

#endif
