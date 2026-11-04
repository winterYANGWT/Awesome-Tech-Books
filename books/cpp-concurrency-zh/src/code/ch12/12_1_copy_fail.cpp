// mode: fail
// expect: deleted
#include <atomic>

int main() {
    std::atomic<int> a{1};
    std::atomic<int> b = a;  // 拷贝构造被删除：会把两个对象的操作合并成一组非原子操作
    b.store(2);
}
