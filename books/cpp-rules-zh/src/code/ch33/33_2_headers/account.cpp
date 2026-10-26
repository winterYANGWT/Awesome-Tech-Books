#include "account.h"

#include <utility>

namespace billing {
Account::Account(std::string owner) : owner_(std::move(owner)) {}

const std::string& Account::owner() const { return owner_; }

Money Account::balance() const { return balance_; }
}
