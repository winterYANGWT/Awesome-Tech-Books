// 第 35.2 节：C++ 实现内部用 std::string，边界上写回调用方的缓冲区
#include "sensor_api.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <string>

extern "C" size_t sensor_format(const sensor_reading* reading, char* buffer, size_t capacity) noexcept {
    try {
        if (reading == nullptr) {
            return 0;
        }

        // 内部继续用高层类型；只有这个导出函数负责和 C 的字节缓冲区打交道。
        const std::string text = std::format("sensor#{}={:.1f}C", reading->id, reading->celsius);
        if (buffer != nullptr && capacity > 0) {
            const size_t n = std::min(capacity - 1, text.size());
            std::memcpy(buffer, text.data(), n);
            buffer[n] = '\0';
        }
        return text.size();
    } catch (...) {
        return static_cast<size_t>(-1);  // 分配失败等异常不穿过 C 边界
    }
}
