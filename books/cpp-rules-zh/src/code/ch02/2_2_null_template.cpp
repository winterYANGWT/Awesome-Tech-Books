// 第 2.2 节：反例：0 被模板推成 int，不能当成空指针用
// mode: fail
// expect: no viable conversion
#include <memory>

template <class Ptr>
void take(Ptr p) {
    // 这里希望 p 是某种空指针；传 nullptr 时 Ptr 是 std::nullptr_t，
    // 传 0 时 Ptr 是 int，下一行就编译不过。
    std::unique_ptr<int> owner = p;
    (void)owner;
}

int main() {
    take(0);
}
