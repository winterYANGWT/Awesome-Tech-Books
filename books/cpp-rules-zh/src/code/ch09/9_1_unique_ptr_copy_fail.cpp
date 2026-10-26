// 第 9.1 节：unique_ptr 的独占所有权写在类型里，拷贝在编译期就被拒绝
// mode: fail
// expect: deleted constructor
#include <memory>
#include <vector>

struct Connection {
    int id = 0;
};

int main() {
    std::unique_ptr<Connection> owner = std::make_unique<Connection>();
    std::unique_ptr<Connection> copy = owner;   // 错误：拷贝构造被删除

    std::vector<std::unique_ptr<Connection>> pool;
    pool.push_back(std::make_unique<Connection>());
    auto snapshot = pool;                      // 错误：vector 的元素不可拷贝

    (void)copy;
    (void)snapshot;
}
