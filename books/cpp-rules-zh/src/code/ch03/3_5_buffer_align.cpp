// 第 3.5 节：字符缓冲区只按 char 对齐，在里面构造 string 不满足对齐要求
// mode: crash
// expect: misaligned
#include <cstddef>
#include <cstdio>
#include <new>
#include <string>

struct Blob {
    char tag;
    unsigned char storage[sizeof(std::string)];
};

// 整个 Blob 只按 1 字节对齐，storage 从偏移 1 开始
static_assert(alignof(Blob) == 1);
static_assert(offsetof(Blob, storage) == 1);

int main() {
    Blob blob{};
    blob.tag = 's';

    // storage 的偏移是 1，Blob 的对齐只有 1，不是 std::string 需要的对齐
    auto* text = new (blob.storage) std::string("hello");
    std::printf("%s\n", text->c_str());
    text->~basic_string();
}
