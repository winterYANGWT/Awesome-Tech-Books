// 第 33.2 节：带外部链接的变量定义，不能放头文件
#ifndef CH33_SHARED_COUNTER_H
#define CH33_SHARED_COUNTER_H

namespace billing {
int refresh_count = 0;   // 每个包含者都会定义一份
}

#endif
