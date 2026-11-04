// mode: run
// 开放消息集：邮箱存 std::any，用类型表在运行期分派；未知类型打印 unknown。
#include <any>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <typeindex>
#include <utility>
#include <vector>

struct set_msg {
    std::string key;
    int value;
};
struct get_msg {
    std::string key;
};
struct del_msg {
    std::string key;
};
struct stop_msg {};

template <class T>
class Mailbox {
public:
    bool send(T value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return false;
            }
            q_.push_back(std::move(value));
        }
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> recv() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        T value = std::move(q_.front());
        q_.pop_front();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable not_empty_;
    std::deque<T> q_;
    bool closed_ = false;
};

class Actor {
public:
    Actor() {
        table_.emplace_back(typeid(set_msg), [this](const std::any& a) {
            const auto& m = std::any_cast<const set_msg&>(a);
            store_[m.key] = m.value;
            std::cout << "set " << m.key << " = " << m.value << '\n';
        });
        table_.emplace_back(typeid(get_msg), [this](const std::any& a) {
            const auto& m = std::any_cast<const get_msg&>(a);
            auto it = store_.find(m.key);
            std::cout << "get " << m.key << " -> ";
            std::cout << (it == store_.end() ? std::string("缺失") : std::to_string(it->second))
                      << '\n';
        });
        table_.emplace_back(typeid(del_msg), [this](const std::any& a) {
            const auto& m = std::any_cast<const del_msg&>(a);
            auto it = store_.find(m.key);
            std::cout << "del " << m.key << " -> ";
            if (it == store_.end()) {
                std::cout << "缺失\n";
                return;
            }
            std::cout << it->second << '\n';
            store_.erase(it);
        });
    }

    // 返回 false 表示收到 stop。
    bool dispatch(const std::any& msg) {
        if (msg.type() == typeid(stop_msg)) {
            std::cout << "stop\n";
            return false;
        }
        for (const auto& [type, handler] : table_) {
            if (msg.type() == type) {
                handler(msg);
                return true;
            }
        }
        std::cout << "unknown\n";
        return true;
    }

private:
    std::map<std::string, int> store_;
    std::vector<std::pair<std::type_index, std::function<void(const std::any&)>>> table_;
};

int main() {
    Mailbox<std::any> inbox;
    Actor actor;
    std::thread worker([&] {
        while (auto msg = inbox.recv()) {
            if (!actor.dispatch(*msg)) {
                break;
            }
        }
    });

    inbox.send(set_msg{"a", 1});
    inbox.send(set_msg{"b", 2});
    inbox.send(get_msg{"a"});
    inbox.send(del_msg{"a"});
    inbox.send(get_msg{"a"});
    inbox.send(std::string("ping"));  // 表里没有的类型
    inbox.send(stop_msg{});
    worker.join();
}
