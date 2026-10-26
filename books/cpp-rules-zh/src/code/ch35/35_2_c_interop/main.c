/* 第 35.2 节：完全用 C 编译的调用方，只依赖 sensor_api.h 里的声明 */
#include "sensor_api.h"

#include <stdio.h>

int main(void) {
    sensor_reading reading = {7, 21.5};
    char small[8];
    char full[64];

    size_t need = sensor_format(&reading, small, sizeof small);
    printf("small=\"%s\" need=%zu\n", small, need);

    need = sensor_format(&reading, full, sizeof full);
    printf("full=\"%s\" need=%zu\n", full, need);
    return 0;
}
