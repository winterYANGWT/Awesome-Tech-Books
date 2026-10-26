// 第 33.6 节：比较两个翻译单元里同名对象的地址
#include "config.h"

#include <iostream>

namespace cfg {
const int* a_max_address();
const int* a_buffer_address();
const int* a_timeout_address();
const int* b_max_address();
const int* b_buffer_address();
const int* b_timeout_address();
}

int main() {
    const int* a_max = cfg::a_max_address();
    const int* b_max = cfg::b_max_address();
    const int* a_buffer = cfg::a_buffer_address();
    const int* b_buffer = cfg::b_buffer_address();
    const int* a_timeout = cfg::a_timeout_address();
    const int* b_timeout = cfg::b_timeout_address();

    std::cout << "kMaxRetries: " << *a_max << ' ' << *b_max
              << " same address: " << (a_max == b_max ? "yes" : "no") << '\n';
    std::cout << "kBufferSize: " << *a_buffer << ' ' << *b_buffer
              << " same address: " << (a_buffer == b_buffer ? "yes" : "no") << '\n';
    std::cout << "kTimeoutMs: " << *a_timeout << ' ' << *b_timeout
              << " same address: " << (a_timeout == b_timeout ? "yes" : "no") << '\n';
    std::cout << "main has its own kMaxRetries: "
              << (&cfg::kMaxRetries != a_max ? "yes" : "no") << '\n';
}
