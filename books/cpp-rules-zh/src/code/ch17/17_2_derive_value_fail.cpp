// 第 17.2 节：给 final 的值类型派生会直接编译失败
// mode: fail
// expect: final
#include <string>

class Text final {
public:
    explicit Text(std::string value) : value_(std::move(value)) {}
    const std::string& str() const { return value_; }

private:
    std::string value_;
};

// 想“加一点功能”就派生，是给具体值类型加功能的典型错误入口。
class ShoutingText : public Text {
public:
    std::string extra() const { return "!"; }
};

int main() {
    return 0;
}
