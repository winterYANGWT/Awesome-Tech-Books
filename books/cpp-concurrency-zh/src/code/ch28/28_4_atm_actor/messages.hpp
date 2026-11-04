#pragma once

#include <string>
#include <variant>

// ATM 收到的输入和银行回复。
struct card_inserted {};
struct digit_pressed {
    int digit;
};
struct withdraw {
    int amount;
};
struct balance_query {};
struct cancel {};
struct pin_ok {};
struct pin_bad {};
struct withdraw_ok {};
struct withdraw_denied {};
struct balance_result {
    int amount;
};

// 三个 actor 共用的停止消息。
struct stop {};

using AtmMsg =
    std::variant<card_inserted, digit_pressed, withdraw, balance_query, cancel, pin_ok, pin_bad,
                 withdraw_ok, withdraw_denied, balance_result, stop>;

// ATM 发给银行的请求。
struct verify_pin {
    int pin;
};
struct bank_withdraw {
    int amount;
};
struct bank_balance {};
using BankMsg = std::variant<verify_pin, bank_withdraw, bank_balance, stop>;

// ATM 发给界面的显示请求。
struct display {
    std::string text;
};
using UiMsg = std::variant<display, stop>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
