#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "mailbox.hpp"
#include "messages.hpp"

class Ui {
public:
    explicit Ui(Mailbox<UiMsg>& inbox);
    ~Ui();

    Mailbox<UiMsg>& mailbox() { return inbox_; }
    int lines() const { return lines_.load(); }
    bool wait_lines(int n);
    void join();

private:
    void run();

    Mailbox<UiMsg>& inbox_;
    std::atomic<int> lines_{0};
    std::thread thread_;
};

class Bank {
public:
    Bank(Mailbox<BankMsg>& inbox, Mailbox<AtmMsg>& atm);
    ~Bank();

    Mailbox<BankMsg>& mailbox() { return inbox_; }
    void join();

private:
    void run();

    Mailbox<BankMsg>& inbox_;
    Mailbox<AtmMsg>& atm_;
    int balance_ = 199;
    std::thread thread_;
};

class Atm {
public:
    Atm(Mailbox<AtmMsg>& inbox, Mailbox<BankMsg>& bank, Mailbox<UiMsg>& ui);
    ~Atm();

    void join();

private:
    enum class State {
        waiting_for_card,
        getting_pin,
        verifying_pin,
        wait_for_action,
        process_withdrawal,
        process_balance,
    };

    void run();
    bool handle(const AtmMsg& msg);
    void say(std::string text);

    Mailbox<AtmMsg>& inbox_;
    Mailbox<BankMsg>& bank_;
    Mailbox<UiMsg>& ui_;
    State state_ = State::waiting_for_card;
    std::string pin_;
    int amount_ = 0;
    std::thread thread_;
};
