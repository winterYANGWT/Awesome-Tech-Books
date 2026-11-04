#include <cstdio>

#include "actors.hpp"
#include "messages.hpp"

int main() {
    Mailbox<UiMsg> ui_inbox(8);
    Mailbox<BankMsg> bank_inbox(8);
    Mailbox<AtmMsg> atm_inbox(8);

    Ui ui(ui_inbox);
    Bank bank(bank_inbox, atm_inbox);
    Atm atm(atm_inbox, bank_inbox, ui_inbox);

    // ANCHOR: drive
    bool ok = true;
    auto expect_lines = [&](int n) {
        if (!ui.wait_lines(n)) {
            ok = false;
        }
    };
    auto send = [&](AtmMsg msg) {
        if (!atm_inbox.send(std::move(msg))) {
            ok = false;
        }
    };

    // 驱动代码模拟按键：每一步等界面把该显示的内容打印出来，再按下一个键。
    send(card_inserted{});
    expect_lines(1);
    for (int digit : {1, 9, 3, 7}) {
        send(digit_pressed{digit});
    }
    expect_lines(3);
    send(withdraw{50});
    expect_lines(5);
    send(balance_query{});
    expect_lines(6);
    send(cancel{});
    expect_lines(7);
    send(stop{});
    expect_lines(8);
    // ANCHOR_END: drive

    atm.join();
    bank.join();
    ui.join();

    std::printf("%s\n", ok ? "驱动完成：8 条界面消息，3 个 actor 都已汇入" : "驱动超时");
}
