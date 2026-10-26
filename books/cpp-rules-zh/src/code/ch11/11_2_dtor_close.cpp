// 第 11.2 节：析构只做不抛的兜底清理，可能失败的关闭留给显式 close()
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

// 模拟一个关闭可能失败的连接；真实代码里可能是数据库、文件或套接字
class Connection {
public:
    void close() {
        ++close_calls_;
        if (fail_next_close_) {
            fail_next_close_ = false;
            throw std::runtime_error("连接关闭失败");
        }
    }
    void fail_next_close() { fail_next_close_ = true; }
    int close_calls() const { return close_calls_; }

private:
    bool fail_next_close_ = false;
    int close_calls_ = 0;
};

class Conn {
public:
    explicit Conn(std::string name) : name_(std::move(name)) {}
    ~Conn() noexcept {                    // 兜底：吞掉异常，只记一笔
        if (!closed_) {
            try {
                conn_.close();
            } catch (const std::exception& e) {
                std::cout << "  ~Conn(" << name_ << ") 兜底关闭失败：" << e.what() << "\n";
                ++swallowed_;
            }
        }
    }

    void close() {                        // 调用方有机会处理失败
        conn_.close();
        closed_ = true;
        ++user_closes_;
    }

    void fail_next_close() { conn_.fail_next_close(); }
    static int swallowed() { return swallowed_; }
    static int user_closes() { return user_closes_; }

private:
    std::string name_;
    Connection conn_;
    bool closed_ = false;
    static inline int swallowed_ = 0;
    static inline int user_closes_ = 0;
};

static_assert(std::is_nothrow_destructible_v<Conn>);

int main() {
    std::cout << "--- 调用方显式 close 并处理失败 ---\n";
    {
        Conn conn("orders");
        conn.fail_next_close();
        try {
            conn.close();                 // 失败从普通函数抛出，调用方接得住
        } catch (const std::exception& e) {
            std::cout << "  调用方捕获：" << e.what() << "，重试一次\n";
            conn.close();                 // 第一次失败时 closed_ 还没置位，重试成功
        }
    }                                     // closed_ 已置位，析构不再重试

    std::cout << "--- 调用方忘了 close ---\n";
    {
        Conn conn("audit");
        conn.fail_next_close();
    }                                     // 析构兜底，异常被吞掉

    std::cout << "user_closes=" << Conn::user_closes()
              << " swallowed=" << Conn::swallowed() << "\n";
}
