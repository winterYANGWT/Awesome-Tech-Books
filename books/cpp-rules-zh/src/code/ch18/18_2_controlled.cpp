// 第 18.2 节：私有继承 + friend，只让指定函数把 Session 当 Codec 用
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

class Codec {
public:
    virtual ~Codec() = default;
    virtual std::string encode(std::string_view text) const = 0;
};

class Session : private Codec {
public:
    explicit Session(std::string key) : key_(std::move(key)) {}

    // 成员函数可以使用私有基类的接口。
    std::string encrypt(const std::string& text) const { return encode(text); }

    // 只有这个友元能把 Session 绑定到 Codec&，其他人不行。
    friend std::string send(const Session& session, const std::string& text) {
        const Codec& codec = session;
        return codec.encode(text);
    }

private:
    std::string encode(std::string_view text) const override {
        std::string result(text.rbegin(), text.rend());
        return key_ + ":" + result;
    }

    std::string key_;
};

int main() {
    Session session("k");
    std::cout << "成员函数 encrypt(hello)=" << session.encrypt("hello") << '\n';
    std::cout << "友元函数 send(hello)=" << send(session, "hello") << '\n';

    std::cout << std::boolalpha
              << "Session is_base_of<Codec>=" << std::is_base_of_v<Codec, Session>
              << '\n'
              << "外部可把 Session& 转成 Codec&="
              << std::is_convertible_v<const Session&, const Codec&> << '\n';

    static_assert(std::is_base_of_v<Codec, Session>);
    static_assert(!std::is_convertible_v<const Session&, const Codec&>);
}
