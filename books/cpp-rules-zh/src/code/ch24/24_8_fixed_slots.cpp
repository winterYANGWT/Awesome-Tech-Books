// 第 24.8 节：固定数组和 std::array 会构造每个槽位，vector 的 reserve 不会
#include <array>
#include <iostream>
#include <optional>
#include <vector>

struct Session {
    int id = 0;
    static inline int constructed = 0;
    static inline int destroyed = 0;

    Session() { ++constructed; }
    ~Session() { ++destroyed; }
};

void reset() {
    Session::constructed = Session::destroyed = 0;
}

void report(const char* what) {
    std::cout << what << ": 构造 " << Session::constructed
              << " 个，析构 " << Session::destroyed << " 个\n";
}

int main() {
    reset();
    {
        Session pool[100];  // TIC-11 的例子：一次性默认构造 100 个
        report("Session pool[100]");
    }
    report("离开作用域后");

    reset();
    {
        std::array<Session, 100> fixed;  // std::array 也是对每个元素默认构造
        report("std::array<Session, 100>");
    }

    reset();
    {
        std::vector<Session> sized(100);  // 带 size 的构造会立刻构造 100 个
        report("std::vector<Session> sized(100)");
    }

    reset();
    {
        std::vector<Session> dynamic;
        dynamic.reserve(100);  // 只分配容量，一个 Session 也不构造
        std::cout << "reserve(100) 后容量 " << dynamic.capacity()
                  << "，元素 " << dynamic.size() << '\n';
        report("vector reserve 之后");

        for (int i = 0; i < 10; ++i) {
            dynamic.emplace_back();  // 真正用到时才构造
        }
        report("emplace_back 10 次之后");
    }

    reset();
    {
        // 定长但按需构造：optional 的默认构造不构造里面的 T
        std::array<std::optional<Session>, 100> lazy;
        for (int i = 0; i < 10; ++i) {
            lazy[i].emplace();
        }
        report("optional 数组 emplace 10 个之后");
    }

    // 需要带符号的差值时，std::ssize 比 size() 的减法安全
    std::vector<int> readings{10, 20, 30};
    std::cout << "std::ssize(readings) - 5 = " << std::ssize(readings) - 5 << '\n';
}
