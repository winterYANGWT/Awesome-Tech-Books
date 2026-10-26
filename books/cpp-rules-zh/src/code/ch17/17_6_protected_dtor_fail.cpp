// 第 17.6 节：protected 非虚析构函数让多态删除在编译期失败
// mode: fail
// expect: protected
#include <iostream>

class Task {
public:
    void run() const { doRun(); }

protected:
    // 不打算让调用方 delete Task*，所以析构既非虚也非公开。
    ~Task() = default;

private:
    virtual void doRun() const = 0;
};

class PrintTask : public Task {
private:
    void doRun() const override { std::cout << "print\n"; }
};

int main() {
    Task* task = new PrintTask;
    delete task;  // Task::~Task 是 protected，编译期拒绝
}
