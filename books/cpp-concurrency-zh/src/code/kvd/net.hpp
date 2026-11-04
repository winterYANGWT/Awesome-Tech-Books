// kvd 的网络小工具：监听、非阻塞、缓冲、端口文件。
//
// 只包 Linux 的系统调用；出错统一抛 std::system_error，错误码就是 errno。
#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace kvd_detail {

[[noreturn]] inline void throw_errno(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}

}  // namespace kvd_detail

inline void set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) kvd_detail::throw_errno("fcntl(F_GETFL)");
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        kvd_detail::throw_errno("fcntl(F_SETFL)");
}

// 阻塞 fd 上返回 true 表示 data 全部写完；EINTR 会重试。
// 非阻塞 fd 上内核缓冲满了返回 false，但调用者不知道已经写进多少，
// 所以事件循环要用 Buffer 加 send 自己记账（kvbench 就是那样做的）。
inline bool write_all(int fd, std::string_view data) {
    while (!data.empty()) {
        ssize_t n = ::write(fd, data.data(), data.size());
        if (n > 0) {
            data.remove_prefix(static_cast<std::size_t>(n));
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;
        if (n == 0) errno = EIO;
        kvd_detail::throw_errno("write");
    }
    return true;
}

// 输入和输出共用的字节缓冲。consume 只移动偏移，不立刻搬数据：
// 部分写之后通常还要把剩下的再写一次，频繁 erase 会把开销做成 O(n²)。
class Buffer {
public:
    void append(std::string_view bytes) {
        if (offset_ == buffer_.size()) {
            buffer_.clear();
            offset_ = 0;
        } else if (offset_ > 4096 && offset_ * 2 > buffer_.size()) {
            buffer_.erase(0, offset_);  // 已消费的前缀太多，才真正搬一次
            offset_ = 0;
        }
        buffer_.append(bytes);
    }

    void consume(std::size_t n) {
        if (n > size()) throw std::out_of_range("Buffer::consume");
        offset_ += n;
        if (offset_ == buffer_.size()) {
            buffer_.clear();
            offset_ = 0;
        }
    }

    std::string_view data() const {
        return std::string_view(buffer_).substr(offset_);
    }
    std::size_t size() const { return buffer_.size() - offset_; }
    bool empty() const { return size() == 0; }
    void clear() {
        buffer_.clear();
        offset_ = 0;
    }

private:
    std::string buffer_;
    std::size_t offset_ = 0;
};

// 等端口文件出现并读出端口。服务器先写临时文件再 rename，
// 所以读到的是一个完整的文件；这里仍然容错地重试到超时。
inline std::uint16_t read_port_file(const char* path,
                                    std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        int fd = ::open(path, O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            char buf[32];
            ssize_t n = 0;
            do {
                n = ::read(fd, buf, sizeof buf);
            } while (n < 0 && errno == EINTR);
            ::close(fd);
            if (n > 0) {
                std::string_view text(buf, static_cast<std::size_t>(n));
                while (!text.empty() &&
                       (text.back() == '\n' || text.back() == '\r' ||
                        text.back() == ' ')) {
                    text.remove_suffix(1);
                }
                unsigned port = 0;
                auto [ptr, ec] = std::from_chars(
                    text.data(), text.data() + text.size(), port);
                if (ec == std::errc{} && ptr == text.data() + text.size() &&
                    port >= 1 && port <= 65535) {
                    return static_cast<std::uint16_t>(port);
                }
            }
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            throw std::system_error(ETIMEDOUT, std::generic_category(),
                                    "read_port_file");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// 监听 127.0.0.1:port；port 为 0 时由内核选一个，写进 port_file（非空时）。
inline int listen_tcp(std::uint16_t port, const char* port_file) {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) kvd_detail::throw_errno("socket");

    auto close_and_throw = [fd](const char* what) {
        int saved = errno;
        ::close(fd);
        throw std::system_error(saved, std::generic_category(), what);
    };

    int yes = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        close_and_throw("setsockopt(SO_REUSEADDR)");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0)
        close_and_throw("bind");
    if (::listen(fd, 128) < 0) close_and_throw("listen");

    if (port_file != nullptr) {
        socklen_t len = sizeof addr;
        if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0)
            close_and_throw("getsockname");
        const std::uint16_t actual = ntohs(addr.sin_port);

        // 先写临时文件，写完再 rename：客户端不会读到半个端口号。
        const std::string tmp =
            std::string(port_file) + ".tmp." + std::to_string(::getpid());
        int pfd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                         0644);
        if (pfd < 0) close_and_throw("open port file");

        const std::string text = std::to_string(actual) + "\n";
        try {
            if (!write_all(pfd, text)) {
                throw std::system_error(errno, std::generic_category(),
                                        "write port file");
            }
        } catch (...) {
            ::close(pfd);
            ::unlink(tmp.c_str());
            ::close(fd);
            throw;
        }
        if (::close(pfd) < 0) {
            int saved = errno;
            ::unlink(tmp.c_str());
            ::close(fd);
            throw std::system_error(saved, std::generic_category(),
                                    "close port file");
        }
        if (::rename(tmp.c_str(), port_file) < 0) {
            int saved = errno;
            ::unlink(tmp.c_str());
            ::close(fd);
            throw std::system_error(saved, std::generic_category(),
                                    "rename port file");
        }
    }
    return fd;
}
