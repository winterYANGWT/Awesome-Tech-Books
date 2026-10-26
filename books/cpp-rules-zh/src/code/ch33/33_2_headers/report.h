// 第 33.2 节：只声明流类型，用 <iosfwd>，不用 <ostream>
#ifndef CH33_REPORT_H
#define CH33_REPORT_H

#include <iosfwd>

namespace billing {
class Account;

std::ostream& operator<<(std::ostream& os, const Account& account);
}

#endif
