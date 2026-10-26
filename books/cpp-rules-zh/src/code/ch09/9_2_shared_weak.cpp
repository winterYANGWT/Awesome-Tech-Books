// 第 9.2 节：shared_ptr 管共享生命周期，weak_ptr 只观察、不延长
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Document {
public:
    explicit Document(std::string title) : title_(std::move(title)) {
        ++live_;
    }
    ~Document() {
        --live_;
        std::cout << "  销毁文档 " << title_ << "（剩余 " << live_ << "）\n";
    }
    const std::string& title() const { return title_; }
    static int live() { return live_; }

private:
    std::string title_;
    static int live_;
};

int Document::live_ = 0;

// 缓存只观察对象：调用方都不用了，文档就销毁，缓存项自然过期。
class DocumentCache {
public:
    std::shared_ptr<const Document> load(int id) {
        if (auto cached = entries_[id].lock()) {
            std::cout << "  命中缓存 #" << id << "\n";
            return cached;
        }
        std::cout << "  未命中，加载 #" << id << "\n";
        auto doc = std::make_shared<const Document>("doc-" + std::to_string(id));
        entries_[id] = doc;
        return doc;
    }

    bool expired(int id) const {
        auto it = entries_.find(id);
        return it != entries_.end() && it->second.expired();
    }

private:
    std::unordered_map<int, std::weak_ptr<const Document>> entries_;
};

class Observer {
public:
    explicit Observer(std::string name) : name_(std::move(name)) {}
    void update(const std::string& message) const {
        std::cout << "  " << name_ << " 收到：" << message << "\n";
    }
    const std::string& name() const { return name_; }

private:
    std::string name_;
};

// 主题不拥有观察者，所以只存 weak_ptr；通知前 lock，观察者已销毁就跳过。
class Subject {
public:
    void subscribe(const std::shared_ptr<Observer>& observer) {
        observers_.push_back(observer);
        std::cout << "  订阅：" << observer->name() << "\n";
    }

    void publish(const std::string& message) {
        int delivered = 0;
        int removed = 0;
        for (auto it = observers_.begin(); it != observers_.end();) {
            if (auto observer = it->lock()) {
                observer->update(message);
                ++delivered;
                ++it;
            } else {
                it = observers_.erase(it);
                ++removed;
            }
        }
        std::cout << "  送达 " << delivered << " 个，清理 " << removed
                  << " 个过期观察者\n";
    }

private:
    std::vector<std::weak_ptr<Observer>> observers_;
};

struct Node {
    explicit Node(std::string n) : name(std::move(n)) {}
    ~Node() { std::cout << "  销毁节点 " << name << "\n"; }
    std::string name;
    std::shared_ptr<Node> next;   // 拥有下一个节点
    std::weak_ptr<Node> back;     // 指回前驱，但不拥有
};

int main() {
    std::cout << "== 缓存 ==\n";
    DocumentCache cache;
    {
        auto first = cache.load(7);
        std::cout << "  first 的引用计数：" << first.use_count() << "\n";
        auto second = cache.load(7);
        std::cout << "  second 的引用计数：" << second.use_count()
                  << "，同一个对象：" << std::boolalpha
                  << (first.get() == second.get()) << "\n";
        std::cout << "  缓存项过期：" << cache.expired(7) << "\n";
    }
    std::cout << "  调用方全部释放后，缓存项过期：" << cache.expired(7)
              << "，存活文档 " << Document::live() << "\n";

    std::cout << "== 观察者 ==\n";
    Subject subject;
    {
        auto console = std::make_shared<Observer>("控制台");
        auto file = std::make_shared<Observer>("日志文件");
        subject.subscribe(console);
        subject.subscribe(file);
        subject.publish("温度 42");
        file.reset();
        subject.publish("温度 43");
    }

    std::cout << "== 循环引用 ==\n";
    {
        auto a = std::make_shared<Node>("A");
        auto b = std::make_shared<Node>("B");
        a->next = b;    // A 拥有 B
        b->back = a;    // B 只观察 A
        std::cout << "  A 的引用计数：" << a.use_count() << "\n";
        a.reset();
        std::cout << "  A 已销毁：" << b->back.expired() << "\n";
        std::cout << "  B 仍存活，引用计数：" << b.use_count() << "\n";
    }

    std::cout << "  存活文档 " << Document::live() << "\n";
}
