// mode: run
// 打印几种结构体的大小、对齐和字段间距：填充把热点字段推到不同的 cache line。
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {

struct Plain {
    long a;
    long b;
};

struct OneAligned {
    alignas(64) long a;
    long b;
};

struct BothAligned {
    alignas(64) long a;
    alignas(64) long b;
};

struct Bitfields {
    unsigned a : 1;
    unsigned b : 1;
};

struct HotCold {
    long key;
    long value;
    char name[48];
};

bool same_line(const void* x, const void* y) {
    return reinterpret_cast<std::uintptr_t>(x) / 64 ==
           reinterpret_cast<std::uintptr_t>(y) / 64;
}

template <typename T>
void show(const char* name, const T& obj) {
    std::printf("%s: sizeof=%zu alignof=%zu\n", name, sizeof(T), alignof(T));
    (void)obj;
}

}  // namespace

// ANCHOR: report
int main() {
    alignas(64) Plain plain{};
    alignas(64) OneAligned one{};
    alignas(64) BothAligned both{};
    alignas(64) HotCold hot{};

    show("Plain", plain);
    std::printf("  a@%zu b@%zu distance=%td same_line=%s\n",
                offsetof(Plain, a), offsetof(Plain, b),
                reinterpret_cast<const char*>(&plain.b) -
                    reinterpret_cast<const char*>(&plain.a),
                same_line(&plain.a, &plain.b) ? "yes" : "no");

    show("OneAligned", one);
    std::printf("  a@%zu b@%zu distance=%td same_line=%s\n",
                offsetof(OneAligned, a), offsetof(OneAligned, b),
                reinterpret_cast<const char*>(&one.b) -
                    reinterpret_cast<const char*>(&one.a),
                same_line(&one.a, &one.b) ? "yes" : "no");

    show("BothAligned", both);
    std::printf("  a@%zu b@%zu distance=%td same_line=%s\n",
                offsetof(BothAligned, a), offsetof(BothAligned, b),
                reinterpret_cast<const char*>(&both.b) -
                    reinterpret_cast<const char*>(&both.a),
                same_line(&both.a, &both.b) ? "yes" : "no");

    show("Bitfields", Bitfields{});
    std::printf("  a 和 b 共用 %zu 字节\n", sizeof(Bitfields));

    show("HotCold", hot);
    std::printf("  key@%zu value@%zu name@%zu same_line(key,name)=%s\n",
                offsetof(HotCold, key), offsetof(HotCold, value), offsetof(HotCold, name),
                same_line(&hot.key, &hot.name) ? "yes" : "no");
    return 0;
}
// ANCHOR_END: report
