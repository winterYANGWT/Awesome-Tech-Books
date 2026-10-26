// 第 14.6 节：final 让“不打算被继承”变成编译期错误
// mode: fail
// expect: final
class Session final {
public:
    virtual ~Session() = default;
};

class AdminSession : public Session {};

int main() {
    AdminSession a;
}
