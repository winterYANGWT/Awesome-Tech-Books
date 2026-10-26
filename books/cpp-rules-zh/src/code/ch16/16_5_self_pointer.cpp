// 第 16.5 节：用代理类让函数自然返回指向自身的函数指针
#include <iostream>
#include <string>

class StatePtr;

using RawState = StatePtr (*)(char);

// 代理类先依赖 RawState，RawState 再依赖 StatePtr，绕开递归 typedef
class StatePtr {
public:
    StatePtr(RawState state) : state_(state) {}

    operator RawState() const { return state_; }

private:
    RawState state_;
};

StatePtr start(char input);
StatePtr running(char input);
StatePtr stopped(char input);

StatePtr start(char input) {
    std::cout << "start(" << input << ")\n";
    return input == 'r' ? running : stopped;
}

StatePtr running(char input) {
    std::cout << "running(" << input << ")\n";
    return input == 'x' ? stopped : running;
}

StatePtr stopped(char input) {
    std::cout << "stopped(" << input << ")\n";
    return stopped;
}

int main() {
    StatePtr state = start;           // 函数名先转成函数指针，再隐式转成 StatePtr
    for (char input : std::string("rxs")) {
        state = state(input);         // 先转成函数指针，再调用；返回值又是 StatePtr
    }
}
