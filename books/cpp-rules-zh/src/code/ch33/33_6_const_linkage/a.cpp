// 第 33.6 节：翻译单元 A 里的 const 副本，以及共享对象的唯一定义
#include "config.h"

namespace cfg {
extern const int kTimeoutMs = 250;

const int* a_max_address() { return &kMaxRetries; }
const int* a_buffer_address() { return &kBufferSize; }
const int* a_timeout_address() { return &kTimeoutMs; }
}
