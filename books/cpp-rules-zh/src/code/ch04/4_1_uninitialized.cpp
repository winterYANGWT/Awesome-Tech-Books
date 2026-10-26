// 第 4.1 节：内置类型不会自动清零，自己初始化自己也是同一个错误
// mode: warn
// expect: -Wuninitialized
struct Timeout {
    int seconds;
    Timeout() : seconds(seconds) {}   // 读的是尚未初始化的成员
};

int read_gain() {
    int gain = gain;                  // 局部变量自己初始化自己
    return gain;
}

int main() {
    Timeout timeout;
    return read_gain() + timeout.seconds;
}
