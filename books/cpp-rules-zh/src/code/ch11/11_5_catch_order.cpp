// 第 11.5 节：catch 子句按出现顺序匹配，基类写在派生类前面会让后者不可达
// mode: warn
// expect: -Wexceptions
#include <iostream>
#include <stdexcept>

int main() {
    try {
        throw std::invalid_argument("数量不能为负");
    } catch (const std::logic_error& e) {          // 先匹配所有 logic_error
        std::cout << "logic_error: " << e.what() << "\n";
    } catch (const std::invalid_argument& e) {     // 永远轮不到
        std::cout << "invalid_argument: " << e.what() << "\n";
    }
}
