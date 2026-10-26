// 第 3.5 节：std::variant 把“一个变量存多种类型”变成可检查的接口
#include <cstdio>
#include <string>
#include <variant>

struct Payment {
    long cents;
};

struct Message {
    std::string text;
};

struct Done {};

using Event = std::variant<Payment, Message, Done>;

struct Describe {
    void operator()(const Payment& payment) const {
        std::printf("payment %ld cents\n", payment.cents);
    }
    void operator()(const Message& message) const {
        std::printf("message: %s\n", message.text.c_str());
    }
    void operator()(Done) const {
        std::printf("done\n");
    }
};

int main() {
    Event events[] = {Payment{1250}, Message{"call back"}, Done{}};
    for (const auto& event : events) {
        std::visit(Describe{}, event);
    }

    Event current = Payment{99};
    std::printf("holds Message: %d\n", std::holds_alternative<Message>(current));

    // 类型不对时抛 bad_variant_access，而不是读另一个成员的位
    try {
        (void)std::get<Message>(current);
    } catch (const std::bad_variant_access&) {
        std::printf("get<Message> failed\n");
    }

    if (auto* message = std::get_if<Message>(&current)) {
        std::printf("message: %s\n", message->text.c_str());
    } else {
        std::printf("get_if<Message> returned null\n");
    }
}
