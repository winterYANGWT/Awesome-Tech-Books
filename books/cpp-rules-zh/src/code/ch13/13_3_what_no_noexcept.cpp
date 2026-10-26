// 第 13.3 节：C++11 起 what() 是 const noexcept，覆盖时不能放宽异常规格
// mode: fail
#include <exception>

class LegacyError : public std::exception {
public:
    // 少了 noexcept：这是 C++98 的写法，在 C++11 之后不是合法的覆盖。
    const char* what() const override { return "legacy error"; }
};

int main() {
    LegacyError error;
    return error.what()[0];
}
