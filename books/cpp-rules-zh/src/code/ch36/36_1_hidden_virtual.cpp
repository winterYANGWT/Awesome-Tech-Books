// 第 36.1 节：漏写 const 的“覆盖”其实是隐藏，override 把它变成编译错误
// mode: fail
// expect: hides virtual member function
struct Notifier {
    virtual ~Notifier() = default;
    virtual void send(int order_id) const = 0;
};

struct EmailNotifier : Notifier {
    // 基类版本是 const，这里漏了 const；override 让编译器指出签名不匹配
    void send(int order_id) override;
};
