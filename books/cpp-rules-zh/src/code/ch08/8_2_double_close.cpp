// 第 8.2 节：资源管理类放任编译器生成拷贝，析构就会执行两次
// mode: crash
// sanitize: none
// expect: 检测到重复关闭
#include <cstdlib>
#include <iostream>
#include <set>

// 模拟一个拥有文件描述符的类。真实代码里重复 close 是未定义行为，
// 这里用 abort 把“同一资源被释放两次”变成确定的失败。
class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}

    ~Socket() {
        // 描述符是进程级资源；用一个集合记住已经关闭过哪些。
        static std::set<int> closed;
        if (!closed.insert(fd_).second) {
            std::cout << "检测到重复关闭" << std::endl;
            std::abort();
        }
        std::cout << "关闭 fd " << fd_ << std::endl;
    }

private:
    int fd_;
};

int main() {
    Socket first(7);
    {
        // 编译器生成的拷贝构造只是逐成员复制 fd_，两个对象都以为
        // 自己拥有 fd 7，析构时就会关闭两次。
        Socket second = first;
        std::cout << "拷贝后两个 Socket 都指向 fd 7" << std::endl;
    }
    std::cout << "second 已析构，first 现在再关闭一次" << std::endl;
}
