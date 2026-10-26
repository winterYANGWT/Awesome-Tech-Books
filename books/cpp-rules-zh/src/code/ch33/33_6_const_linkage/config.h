// 第 33.6 节：命名空间作用域的 const 默认内部链接
#ifndef CH33_CONFIG_H
#define CH33_CONFIG_H

namespace cfg {
const int kMaxRetries = 3;              // 每个翻译单元各一份
inline constexpr int kBufferSize = 4096; // C++17：全程序唯一的一份
extern const int kTimeoutMs;            // 声明；定义在 a.cpp
}

#endif
