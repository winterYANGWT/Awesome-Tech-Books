// 第 33.6 节：翻译单元 B 里的 const 副本
#include "config.h"

namespace cfg {
const int* b_max_address() { return &kMaxRetries; }
const int* b_buffer_address() { return &kBufferSize; }
const int* b_timeout_address() { return &kTimeoutMs; }
}
