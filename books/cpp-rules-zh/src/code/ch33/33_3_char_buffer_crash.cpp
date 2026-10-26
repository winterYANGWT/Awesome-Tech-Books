// 第 33.3 节：用 char 缓冲区手工放对象会丢对齐
// mode: crash
// expect: misaligned address
#include <new>

struct alignas(32) Payload {
    double values[4];
};

int main() {
    alignas(32) char storage[sizeof(Payload) + 1];
    Payload* payload = new (storage + 1) Payload{};  // 偏移 1 字节，不再 32 字节对齐
    payload->values[0] = 1.0;
    return static_cast<int>(payload->values[0]);
}
