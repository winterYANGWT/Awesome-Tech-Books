// 第 7.3 节：关掉复制消除后，具名返回和 return std::move 都只剩一次移动
// flags: -fno-elide-constructors -Wno-pessimizing-move
#include <iostream>
#include <string>
#include <utility>

struct Payload {
    static int copies;
    static int moves;
    std::string text;

    explicit Payload(std::string value) : text(std::move(value)) {}
    Payload(const Payload& other) : text(other.text) { ++copies; }
    Payload(Payload&& other) noexcept : text(std::move(other.text)) { ++moves; }
};
int Payload::copies = 0;
int Payload::moves = 0;

void reset() {
    Payload::copies = 0;
    Payload::moves = 0;
}

Payload make_prvalue() {
    return Payload("prvalue");
}

Payload make_named() {
    Payload local("named");
    return local;  // NRVO 没发生时，标准要求按右值返回（隐式 move）
}

Payload make_named_move() {
    Payload local("named-move");
    return std::move(local);  // 显式 move 阻止 NRVO
}

int main() {
    reset();
    Payload a = make_prvalue();
    std::cout << "纯右值返回：拷贝 " << Payload::copies
              << "，移动 " << Payload::moves << "\n";

    reset();
    Payload b = make_named();
    std::cout << "具名局部变量：拷贝 " << Payload::copies
              << "，移动 " << Payload::moves << "\n";

    reset();
    Payload c = make_named_move();
    std::cout << "return std::move(局部)：拷贝 " << Payload::copies
              << "，移动 " << Payload::moves << "\n";

    std::cout << "内容：" << a.text << " / " << b.text << " / " << c.text << "\n";
}
