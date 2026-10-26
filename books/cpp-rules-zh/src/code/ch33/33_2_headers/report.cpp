#include "report.h"
#include "account.h"

#include <ostream>

namespace billing {
std::ostream& operator<<(std::ostream& os, const Account& account) {
    return os << account.owner() << ':' << account.balance().cents
              << ' ' << account.balance().currency;
}
}
