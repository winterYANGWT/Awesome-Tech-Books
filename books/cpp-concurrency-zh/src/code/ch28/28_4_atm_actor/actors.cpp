#include "actors.hpp"

#include <chrono>
#include <iostream>
#include <utility>
#include <variant>

// ---------------- Ui ----------------

Ui::Ui(Mailbox<UiMsg>& inbox) : inbox_(inbox), thread_([this] { run(); }) {}

Ui::~Ui() {
    inbox_.close();
    join();
}

void Ui::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool Ui::wait_lines(int n) {
    for (int i = 0; i < 2000 && lines_.load() < n; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return lines_.load() >= n;
}

void Ui::run() {
    while (auto msg = inbox_.recv()) {
        if (std::holds_alternative<stop>(*msg)) {
            break;
        }
        std::cout << std::get<display>(*msg).text << '\n' << std::flush;
        ++lines_;
    }
}

// ---------------- Bank ----------------

Bank::Bank(Mailbox<BankMsg>& inbox, Mailbox<AtmMsg>& atm)
    : inbox_(inbox), atm_(atm), thread_([this] { run(); }) {}

Bank::~Bank() {
    inbox_.close();
    join();
}

void Bank::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Bank::run() {
    while (auto msg = inbox_.recv()) {
        bool keep = std::visit(overloaded{
                                   [&](const verify_pin& m) {
                                       atm_.send(m.pin == 1937 ? AtmMsg{pin_ok{}}
                                                               : AtmMsg{pin_bad{}});
                                       return true;
                                   },
                                   [&](const bank_withdraw& m) {
                                       if (m.amount <= balance_) {
                                           balance_ -= m.amount;
                                           atm_.send(AtmMsg{withdraw_ok{}});
                                       } else {
                                           atm_.send(AtmMsg{withdraw_denied{}});
                                       }
                                       return true;
                                   },
                                   [&](const bank_balance&) {
                                       atm_.send(AtmMsg{balance_result{balance_}});
                                       return true;
                                   },
                                   [&](const stop&) { return false; },
                               },
                               *msg);
        if (!keep) {
            break;
        }
    }
}

// ---------------- Atm ----------------

Atm::Atm(Mailbox<AtmMsg>& inbox, Mailbox<BankMsg>& bank, Mailbox<UiMsg>& ui)
    : inbox_(inbox), bank_(bank), ui_(ui), thread_([this] { run(); }) {}

Atm::~Atm() {
    inbox_.close();
    join();
}

void Atm::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Atm::say(std::string text) {
    ui_.send(display{std::move(text)});
}

void Atm::run() {
    while (auto msg = inbox_.recv()) {
        if (!handle(*msg)) {
            break;
        }
    }
}

bool Atm::handle(const AtmMsg& msg) {
    return std::visit(overloaded{
                          [&](const card_inserted&) {
                              if (state_ != State::waiting_for_card) {
                                  return true;
                              }
                              pin_.clear();
                              state_ = State::getting_pin;
                              say("插卡成功，请输入 PIN");
                              return true;
                          },
                          [&](const digit_pressed& m) {
                              if (state_ != State::getting_pin) {
                                  return true;
                              }
                              pin_.push_back(static_cast<char>('0' + m.digit));
                              if (pin_.size() == 4) {
                                  state_ = State::verifying_pin;
                                  say("正在验证 PIN…");
                                  bank_.send(verify_pin{std::stoi(pin_)});
                              }
                              return true;
                          },
                          [&](const pin_ok&) {
                              if (state_ != State::verifying_pin) {
                                  return true;
                              }
                              state_ = State::wait_for_action;
                              say("PIN 正确，请选择操作");
                              return true;
                          },
                          [&](const pin_bad&) {
                              if (state_ != State::verifying_pin) {
                                  return true;
                              }
                              pin_.clear();
                              state_ = State::getting_pin;
                              say("PIN 错误，请重新输入");
                              return true;
                          },
                          [&](const withdraw& m) {
                              if (state_ != State::wait_for_action) {
                                  return true;
                              }
                              amount_ = m.amount;
                              state_ = State::process_withdrawal;
                              say("正在处理取款 " + std::to_string(amount_) + " 元");
                              bank_.send(bank_withdraw{amount_});
                              return true;
                          },
                          [&](const withdraw_ok&) {
                              if (state_ != State::process_withdrawal) {
                                  return true;
                              }
                              say("请取走 " + std::to_string(amount_) + " 元");
                              state_ = State::wait_for_action;
                              return true;
                          },
                          [&](const withdraw_denied&) {
                              if (state_ != State::process_withdrawal) {
                                  return true;
                              }
                              say("余额不足");
                              state_ = State::wait_for_action;
                              return true;
                          },
                          [&](const balance_query&) {
                              if (state_ != State::wait_for_action) {
                                  return true;
                              }
                              state_ = State::process_balance;
                              bank_.send(bank_balance{});
                              return true;
                          },
                          [&](const balance_result& m) {
                              if (state_ != State::process_balance) {
                                  return true;
                              }
                              say("余额 " + std::to_string(m.amount) + " 元");
                              state_ = State::wait_for_action;
                              return true;
                          },
                          [&](const cancel&) {
                              if (state_ == State::waiting_for_card) {
                                  return true;
                              }
                              say("请取卡");
                              state_ = State::waiting_for_card;
                              return true;
                          },
                          [&](const stop&) {
                              say("交易结束，欢迎再来");
                              bank_.send(stop{});
                              ui_.send(stop{});
                              return false;
                          },
                      },
                      msg);
}
