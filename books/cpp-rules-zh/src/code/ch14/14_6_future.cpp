// 第 14.6 节：把“不继承、不拷贝、必须显式传参”写进类型
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

struct Token {
    std::string value;
};

class Session final {
public:
    explicit Session(Token token) : token_(std::move(token.value)) {}

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void refresh() {
        ++generation_;
        std::cout << "refresh " << token_ << " gen=" << generation_ << "\n";
    }
    [[nodiscard]] int generation() const { return generation_; }

private:
    std::string token_;
    int generation_ = 0;
};

static_assert(std::is_final_v<Session>);
static_assert(!std::is_copy_constructible_v<Session>);
static_assert(!std::is_copy_assignable_v<Session>);
static_assert(!std::is_move_constructible_v<Session>);   // 删除拷贝也压掉了隐式移动
static_assert(!std::is_convertible_v<Token, Session>);   // explicit 挡住隐式转换
static_assert(std::is_constructible_v<Session, Token>);

int main() {
    Session s(Token{"abc"});
    s.refresh();
    s.refresh();
    std::cout << "gen=" << s.generation() << "\n";
}
