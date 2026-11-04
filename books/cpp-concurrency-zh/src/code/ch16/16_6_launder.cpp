// mode: run
#include <cstdio>
#include <new>

struct Config {
    const int version;
    int value;
};

int main() {
    alignas(Config) unsigned char storage[sizeof(Config)];

    const Config* first = new (storage) const Config{1, 100};
    std::printf("第一次: version=%d value=%d\n", first->version, first->value);

    first->~Config();
    const Config* fresh = new (storage) const Config{2, 200};

    // 完整对象是 const，不能透明替换：first 仍指向旧对象，必须 launder。
    const Config* reused = std::launder(first);

    std::printf("重新构造: version=%d value=%d\n", fresh->version, fresh->value);
    std::printf("std::launder: version=%d value=%d\n", reused->version, reused->value);
    std::printf("地址相同: %s\n",
                reinterpret_cast<const void*>(first) == reinterpret_cast<const void*>(fresh) ? "是" : "否");
    return 0;
}
