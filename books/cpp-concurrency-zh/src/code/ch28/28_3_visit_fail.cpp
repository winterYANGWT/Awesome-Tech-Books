// mode: fail
// expect: no matching function
// 反例：访问器漏掉 stop_msg，std::visit 找不到可调用的重载。
#include <string>
#include <variant>

struct set_msg {
    std::string key;
    int value;
};
struct get_msg {
    std::string key;
};
struct del_msg {
    std::string key;
};
struct stop_msg {};

using Msg = std::variant<set_msg, get_msg, del_msg, stop_msg>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

int main() {
    Msg msg = get_msg{"a"};
    std::visit(overloaded{
                   [](const set_msg&) {},
                   [](const get_msg&) {},
                   [](const del_msg&) {},
               },
               msg);
}
