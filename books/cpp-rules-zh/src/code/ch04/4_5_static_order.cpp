// 第 4.5 节：用函数内 static 把“谁先构造”变成“谁先调用”
#include <iostream>
#include <string>
#include <vector>

class Registry {
public:
    // C++11 起，函数内 static 的初始化是线程安全的；第一次调用时构造
    static Registry& instance() {
        static Registry registry;
        return registry;
    }

    void record(const std::string& event) {
        events_.push_back(event);
        std::cout << "  记录: " << event << "\n";
    }

    ~Registry() {
        std::cout << "Registry 析构，共 " << events_.size() << " 条记录\n";
    }

private:
    Registry() { std::cout << "Registry 构造\n"; }
    std::vector<std::string> events_;
};

// 命名空间级对象：它的构造函数依赖 Registry，但两者在不同翻译单元时，
// 谁先构造并没有保证；instance() 把顺序变成了显式调用依赖。
struct Auditor {
    Auditor() {
        std::cout << "Auditor 构造\n";
        Registry::instance().record("auditor ready");
    }
    ~Auditor() {
        std::cout << "Auditor 析构\n";
        Registry::instance().record("auditor done");
    }
};

Auditor auditor;

// constinit 要求常量初始化：不满足就编译失败，不会留到运行期
constinit int max_sessions = 64;

int main() {
    std::cout << "main 开始，max_sessions = " << max_sessions << "\n";
    Registry::instance().record("main");
}
