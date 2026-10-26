// 第 33.3 节：析构函数在头文件里 = default，Impl 还不完整
#ifndef CH33_BAD_SESSION_H
#define CH33_BAD_SESSION_H

#include <memory>

namespace net {
class Session {
public:
    Session();
    ~Session() = default;   // 错：这里就生成析构，Impl 还没定义
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
#endif
