// 第 36.1 节：清零警告后的同一段逻辑，关键签名由 override 和 [[maybe_unused]] 固定
// mode: run
// flags: -Wshadow
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Message {
    int order_id;
    std::string body;
};

class Notifier {
public:
    virtual ~Notifier() = default;
    virtual void deliver(const Message& message) const = 0;
};

class EmailNotifier : public Notifier {
public:
    void deliver(const Message& message) const override {
        std::cout << "email #" << message.order_id << ": " << message.body << '\n';
    }
};

class SmsNotifier : public Notifier {
public:
    // 参数在 SMS 渠道里用不到，但签名由接口固定；[[maybe_unused]] 比删掉名字更能说明意图
    void deliver([[maybe_unused]] const Message& message) const override {
        std::cout << "sms #" << message.order_id << ": 已发送\n";
    }
};

int main() {
    std::vector<std::unique_ptr<Notifier>> notifiers;
    notifiers.push_back(std::make_unique<EmailNotifier>());
    notifiers.push_back(std::make_unique<SmsNotifier>());

    const Message message{42, "订单已出库"};
    for (const auto& notifier : notifiers) {
        notifier->deliver(message);
    }
}
