// 第 14.2 节：返回内部缓冲区的视图，临时对象销毁后视图悬空
// mode: crash
// expect: heap-use-after-free
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

class Label {
public:
    explicit Label(std::string text) : text_(std::move(text)) {}

    // 视图指向 text_ 的缓冲区；它的有效期跟着 Label 对象走。
    [[nodiscard]] std::string_view view() const { return text_; }

private:
    std::string text_;
};

Label makeLabel() { return Label("invoice-2026-0001-abcdefghijklmnop"); }

int main() {
    // 临时 Label 在这条语句结束时销毁，view 指向的缓冲区随之释放。
    std::string_view v = makeLabel().view();
    std::cout << "size=" << v.size() << "\n";
    std::cout << "text=";
    for (char c : v) std::cout << c;   // 逐字符读已释放的缓冲区
    std::cout << "\n";
}
