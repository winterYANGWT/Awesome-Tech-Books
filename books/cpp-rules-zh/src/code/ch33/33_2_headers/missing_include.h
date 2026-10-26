// 第 33.2 节：故意漏掉 <string>，单独编译这个头文件就会失败
#ifndef CH33_MISSING_INCLUDE_H
#define CH33_MISSING_INCLUDE_H

namespace billing {
struct Note {
    std::string text;   // 没有包含 <string>
};
}

#endif
