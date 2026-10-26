// 第 16.3 节：重载 &&、||、逗号之后，它们退化成普通函数调用
#include <iostream>
#include <string>
#include <utility>

bool check(const char* name, bool value) {
    std::cout << "  求值 " << name << '\n';
    return value;
}

struct Guard {
    std::string name;
    bool ok;
};

Guard make(std::string name, bool ok) {
    std::cout << "  构造 " << name << '\n';
    return {std::move(name), ok};
}

bool operator&&(const Guard& lhs, const Guard& rhs) {
    std::cout << "  operator&&(" << lhs.name << ", " << rhs.name << ")\n";
    return lhs.ok && rhs.ok;
}

bool operator||(const Guard& lhs, const Guard& rhs) {
    std::cout << "  operator||(" << lhs.name << ", " << rhs.name << ")\n";
    return lhs.ok || rhs.ok;
}

struct Tag {
    std::string name;
};

Tag tag(std::string name) {
    std::cout << "  求值 tag " << name << '\n';
    return {std::move(name)};
}

void operator,(const Tag& lhs, const Tag& rhs) {
    std::cout << "  operator,(" << lhs.name << ", " << rhs.name << ")\n";
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "内置 &&：\n";
    bool builtin_and = check("左", false) && check("右", true);
    std::cout << "  结果 " << builtin_and << '\n';

    std::cout << "重载 &&：\n";
    bool overloaded_and = make("左", false) && make("右", true);
    std::cout << "  结果 " << overloaded_and << '\n';

    std::cout << "内置 ||：\n";
    bool builtin_or = check("左", true) || check("右", false);
    std::cout << "  结果 " << builtin_or << '\n';

    std::cout << "重载 ||：\n";
    bool overloaded_or = make("左", true) || make("右", false);
    std::cout << "  结果 " << overloaded_or << '\n';

    std::cout << "内置逗号：\n";
    (check("左", true), check("右", true));

    std::cout << "重载逗号：\n";
    (tag("左"), tag("右"));
}
