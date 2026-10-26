// 第 10.5 节：delete void* 只释放存储，不调用析构函数
// mode: crash
// flags: -Wno-delete-incomplete
// expect: LeakSanitizer
#include <iostream>

struct Payload {
    int values[16]{};
};

class Session {
public:
    Session() : data_(new Payload{}) {}
    ~Session() {
        delete data_;
        std::cout << "  Session 析构，Payload 已释放\n";
    }

private:
    Payload* data_;
};

int main() {
    void* erased = new Session;
    std::cout << "通过 void* 删除 Session\n";
    delete erased;   // 静态类型是 void：不调用 ~Session()
    std::cout << "delete 返回，但 ~Session 没有执行\n";
}
