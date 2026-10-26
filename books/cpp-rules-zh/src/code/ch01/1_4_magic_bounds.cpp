// 第 1.4 节：同一个通道数抄成两个值，越界写入
// mode: crash
// expect: out of bounds

#include <cstdio>

int main() {
    constexpr int kChannels = 4;
    int samples[kChannels]{};

    // 别处抄来的“魔法数”，和 kChannels 已经不一致；volatile 让它在运行期才读。
    volatile int configured = 6;
    for (int i = 0; i < configured; ++i) {
        samples[i] = i;
    }

    std::printf("%d\n", samples[0]);
}
