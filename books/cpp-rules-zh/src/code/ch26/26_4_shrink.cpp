// 第 26.4 节：erase/clear 不释放容量；真正缩容用 swap 惯用法或 shrink_to_fit
#include <cstdio>
#include <vector>

int main() {
    std::vector<int> log(1000, 7);
    std::printf("初始 size=%zu capacity=%zu\n", log.size(), log.capacity());

    log.erase(log.begin() + 10, log.end());
    std::printf("erase 后 size=%zu capacity=%zu\n", log.size(), log.capacity());

    log.shrink_to_fit();  // 非强制请求，标准允许实现忽略
    std::printf("shrink_to_fit 后 size=%zu capacity=%zu（本机 libstdc++）\n",
                log.size(), log.capacity());

    log.assign(1000, 1);
    log.clear();
    std::printf("重新填满再 clear 后 size=%zu capacity=%zu\n", log.size(), log.capacity());

    // 拷贝一份只申请够用的容量，再和原容器交换；旧缓冲区随临时对象销毁
    std::vector<int>(log).swap(log);
    std::printf("swap 惯用法后 size=%zu capacity=%zu\n", log.size(), log.capacity());
}
