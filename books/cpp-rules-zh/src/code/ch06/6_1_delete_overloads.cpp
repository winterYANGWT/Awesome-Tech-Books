// 第 6.1 节：= delete 在编译期拒绝调用，错误落在调用点
// mode: fail
// expect: deleted function
#include <string>

bool isLucky(int number);
bool isLucky(char) = delete;
bool isLucky(bool) = delete;
bool isLucky(double) = delete;

class Session {
public:
    Session() = default;
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    void open() {}
};

int main() {
    isLucky('a');      // 选中被删除的 char 重载
    isLucky(true);     // 选中被删除的 bool 重载
    isLucky(3.5f);     // float 优先转 double，而不是转 int

    Session first;
    Session second(first);   // 拷贝构造被删除
    second = first;          // 拷贝赋值被删除
}
