// 第 19.3 节：枚举 switch 漏值会被 -Wswitch 抓到，但带 default 的 switch 不会
// mode: warn
// expect: -Wswitch
#include <iostream>

enum class Method {
    Card,
    Cash,
    Crypto,
};

const char* name(Method method) {
    switch (method) {
    case Method::Card:
        return "card";
    case Method::Cash:
        return "cash";
    }
    return "unknown";
}

int main() {
    std::cout << name(Method::Crypto) << '\n';
}
