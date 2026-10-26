// 第 8.1 节：手写释放为什么会漏掉路径，RAII 怎样把路径收拢
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

// 一个不是内存的资源：会议室的租约。acquire 与 release 必须配对。
struct LeasePool {
    static std::set<int> open;
    static int next_id;

    static int acquire(const std::string& who) {
        const int id = next_id++;
        open.insert(id);
        std::cout << "  获取租约 " << id << "（" << who << "）\n";
        return id;
    }

    static void release(int id) {
        if (open.erase(id) == 0) {
            std::cout << "  释放了不存在的租约 " << id << "\n";
            return;
        }
        std::cout << "  释放租约 " << id << "\n";
    }
};

std::set<int> LeasePool::open;
int LeasePool::next_id = 1;

// 手写版：提前 return 时 release 永远不会执行。
int use_manual_early_return() {
    const int lease = LeasePool::acquire("manual/early-return");
    if (lease > 0) {
        return 1;
    }
    LeasePool::release(lease);
    return 0;
}

// 手写版：异常越过 release 时同样不会执行。
void use_manual_throw() {
    const int lease = LeasePool::acquire("manual/throw");
    throw std::runtime_error("演示异常");
    LeasePool::release(lease);
}

class Lease {
public:
    explicit Lease(const std::string& who) : id_(LeasePool::acquire(who)) {}
    ~Lease() { LeasePool::release(id_); }

    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;

    int id() const { return id_; }

private:
    int id_;
};

int use_raii_early_return() {
    Lease lease("raii/early-return");
    if (lease.id() > 0) {
        return 1;
    }
    return 0;
}

void use_raii_throw() {
    Lease lease("raii/throw");
    throw std::runtime_error("演示异常");
}

int main() {
    std::cout << "手写 release：\n";
    use_manual_early_return();
    std::cout << "提前 return 后仍打开 " << LeasePool::open.size() << " 个\n";

    try {
        use_manual_throw();
    } catch (const std::exception& e) {
        std::cout << "捕获：" << e.what() << "\n";
    }
    std::cout << "异常之后仍打开 " << LeasePool::open.size() << " 个\n";

    // 演示程序无法再找回那两个 id，只能按登记表手工清理。
    while (!LeasePool::open.empty()) {
        LeasePool::release(*LeasePool::open.begin());
    }

    std::cout << "RAII：\n";
    use_raii_early_return();
    std::cout << "提前 return 后仍打开 " << LeasePool::open.size() << " 个\n";

    try {
        use_raii_throw();
    } catch (const std::exception& e) {
        std::cout << "捕获：" << e.what() << "\n";
    }
    std::cout << "异常之后仍打开 " << LeasePool::open.size() << " 个\n";
}
