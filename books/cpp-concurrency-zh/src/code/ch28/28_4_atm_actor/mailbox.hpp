#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

// 有界邮箱：send 在满时等待，recv 在空且未关闭时等待，close 幂等。
template <class T>
class Mailbox {
public:
    explicit Mailbox(std::size_t capacity) : capacity_(capacity) {}

    Mailbox(const Mailbox&) = delete;
    Mailbox& operator=(const Mailbox&) = delete;

    bool send(T value) {
        std::unique_lock<std::mutex> lock(m_);
        not_full_.wait(lock, [&] { return q_.size() < capacity_ || closed_; });
        if (closed_) {
            return false;
        }
        q_.push_back(std::move(value));
        lock.unlock();
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
        lock.unlock();
        not_full_.notify_one();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> q_;
    std::size_t capacity_;
    bool closed_ = false;
};
