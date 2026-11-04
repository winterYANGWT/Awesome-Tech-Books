// mode: run
// 封闭消息集：邮箱存 std::variant，std::visit 在编译期检查每条分支。
#include <condition_variable>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <variant>

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
using Msg = std::variant<set_msg, get_msg, del_msg, stop_msg>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

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

int main() {
    Mailbox<Msg> inbox;
    std::thread actor([&] {
        std::map<std::string, int> store;
        while (auto msg = inbox.recv()) {
            bool stop = std::visit(overloaded{
                                       [&](const set_msg& m) {
                                           store[m.key] = m.value;
                                           std::cout << "set " << m.key << " = " << m.value << '\n';
                                           return false;
                                       },
                                       [&](const get_msg& m) {
                                           auto it = store.find(m.key);
                                           std::cout << "get " << m.key << " -> ";
                                           if (it == store.end()) {
                                               std::cout << "缺失\n";
                                           } else {
                                               std::cout << it->second << '\n';
                                           }
                                           return false;
                                       },
                                       [&](const del_msg& m) {
                                           auto it = store.find(m.key);
                                           std::cout << "del " << m.key << " -> ";
                                           if (it == store.end()) {
                                               std::cout << "缺失\n";
                                               return false;
                                           }
                                           std::cout << it->second << '\n';
                                           store.erase(it);
                                           return false;
                                       },
                                       [&](const stop_msg&) {
                                           std::cout << "stop\n";
                                           return true;
                                       },
                                   },
                                   *msg);
            if (stop) {
                break;
            }
        }
    });

    inbox.send(set_msg{"a", 1});
    inbox.send(set_msg{"b", 2});
    inbox.send(get_msg{"a"});
    inbox.send(del_msg{"a"});
    inbox.send(get_msg{"a"});
    inbox.send(stop_msg{});
    actor.join();
}
