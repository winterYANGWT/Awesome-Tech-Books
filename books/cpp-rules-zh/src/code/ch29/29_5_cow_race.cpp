// 第 29.5 节：非原子的引用计数在多个线程里同时拷贝时发生数据竞争
// mode: crash
// sanitize: thread
// expect: ThreadSanitizer
#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct Buffer {
    explicit Buffer(std::string text) : text(std::move(text)) {}
    int refs = 1;              // 故意用普通 int：COW 自己的计数
    std::string text;
};

class CowText {
public:
    explicit CowText(std::string text) : buf_(new Buffer(std::move(text))) {}

    CowText(const CowText& other) : buf_(other.buf_) {
        ++buf_->refs;          // 两个线程同时拷贝时，这里竞争
    }

    CowText& operator=(const CowText&) = delete;

    ~CowText() {
        // 为了不让计数竞争把对象提前 delete、变成另一种崩溃，这里只减不删；
        // 真正要展示的是 refs 上的数据竞争，而不是释放策略。
        --buf_->refs;
    }

    const std::string& str() const { return buf_->text; }

private:
    Buffer* buf_;
};

int main() {
    CowText shared("shared-buffer");
    std::atomic<bool> start{false};

    auto worker = [&] {
        while (!start.load(std::memory_order_acquire)) {
        }
        std::vector<CowText> locals;
        locals.reserve(100);
        for (int i = 0; i < 100; ++i) {
            locals.push_back(shared);   // 每个线程都递增共享 refs
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    start.store(true, std::memory_order_release);
    t1.join();
    t2.join();

    std::cout << "done: " << shared.str() << '\n';
}
