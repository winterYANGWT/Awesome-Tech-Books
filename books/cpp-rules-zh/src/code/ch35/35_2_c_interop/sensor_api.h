// 第 35.2 节：C++ 实现、C 调用的传感器接口
#ifndef SENSOR_API_H
#define SENSOR_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#define SENSOR_NOEXCEPT noexcept
#else
#define SENSOR_NOEXCEPT
#endif

typedef struct sensor_reading {
    int id;
    double celsius;
} sensor_reading;

/* 返回不含结尾 '\0' 的所需长度；容量不足时截断并补 '\0'；分配失败返回 (size_t)-1。 */
size_t sensor_format(const sensor_reading* reading, char* buffer, size_t capacity) SENSOR_NOEXCEPT;

#undef SENSOR_NOEXCEPT

#ifdef __cplusplus
}
#endif

#endif
