// 第 9.1 节：用运行期 bool 表达“是否拥有元素”，两个容器会各自 delete 一次
// mode: crash
// expect: AddressSanitizer
#include <iostream>
#include <vector>

struct Reading {
    int value;
    ~Reading() { std::cout << "  析构 Reading " << value << "\n"; }
};

class PtrBag {
public:
    explicit PtrBag(bool owns = true) : owns_(owns) {}
    ~PtrBag() {
        if (!owns_) {
            return;
        }
        for (Reading* item : items_) {
            delete item;
        }
    }
    void add(Reading* item) { items_.push_back(item); }

private:
    std::vector<Reading*> items_;
    bool owns_;
};

int main() {
    Reading* first = new Reading{1};
    Reading* second = new Reading{2};

    std::cout << "两个 PtrBag 都按默认值 owns=true 接收同一批元素\n";
    {
        PtrBag primary;   // 默认 owns=true
        PtrBag replica;   // 本意是“只看一眼”，却忘了传 false
        primary.add(first);
        primary.add(second);
        replica.add(first);
        replica.add(second);
        std::cout << "两个容器都认为自己负责 delete\n";
    }

    std::cout << "如果还能走到这里，说明没有触发检查\n";
}
