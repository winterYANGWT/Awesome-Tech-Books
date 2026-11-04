// kvbench：单线程 epoll 的 kvd 压测客户端。
//
// 每个连接只碰自己的 key（c<编号>-p<进程号>-k<号>），在本地记下期望值；
// 服务器按连接顺序处理请求，所以即使开了流水线，每个响应对应哪个请求也是确定的。
#include "net.hpp"
#include "protocol.hpp"

#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct Args {
    std::uint16_t port = 0;
    std::string port_file;
    int conns = 8;
    int per_conn = 1000;
    double set_ratio = 0.1;
    int keys = 100;
    int pipeline = 1;
    int timeout_seconds = 10;
    bool csv = false;
    bool quiet = false;
    bool has_once = false;
    std::string once;
};

[[noreturn]] void usage_error(const std::string& message) {
    std::cerr << "kvbench: " << message << "\n"
              << "usage: kvbench (--port P | --port-file F) [options]\n"
              << "       kvbench (--port P | --port-file F) --once \"SET a 1\"\n"
              << "options: -c N -n N --set-ratio R --keys N --pipeline N"
                 " --timeout S --csv --quiet\n";
    std::exit(2);
}

int parse_int(const char* text, const char* name) {
    int value = 0;
    const char* end = text + std::strlen(text);
    auto [ptr, ec] = std::from_chars(text, end, value);
    if (ec != std::errc{} || ptr != end) {
        usage_error(std::string(name) + " needs an integer");
    }
    return value;
}

double parse_double(const char* text, const char* name) {
    char* end = nullptr;
    const double value = std::strtod(text, &end);
    if (end == text || *end != '\0') {
        usage_error(std::string(name) + " needs a number");
    }
    return value;
}

Args parse_args(int argc, char** argv) {
    Args args;
    bool has_port = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto value = [&](const char* name) -> const char* {
            if (i + 1 >= argc) usage_error(std::string(name) + " needs a value");
            return argv[++i];
        };

        if (arg == "--port") {
            const int port = parse_int(value("--port"), "--port");
            if (port < 1 || port > 65535) usage_error("port out of range");
            args.port = static_cast<std::uint16_t>(port);
            has_port = true;
        } else if (arg == "--port-file") {
            args.port_file = value("--port-file");
        } else if (arg == "-c" || arg == "--conns") {
            args.conns = parse_int(value("-c"), "-c");
        } else if (arg == "-n") {
            args.per_conn = parse_int(value("-n"), "-n");
        } else if (arg == "--set-ratio") {
            args.set_ratio = parse_double(value("--set-ratio"), "--set-ratio");
        } else if (arg == "--keys") {
            args.keys = parse_int(value("--keys"), "--keys");
        } else if (arg == "--pipeline") {
            args.pipeline = parse_int(value("--pipeline"), "--pipeline");
        } else if (arg == "--timeout") {
            args.timeout_seconds = parse_int(value("--timeout"), "--timeout");
        } else if (arg == "--csv") {
            args.csv = true;
        } else if (arg == "--quiet") {
            args.quiet = true;
        } else if (arg == "--once") {
            args.has_once = true;
            args.once = value("--once");
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "usage: kvbench (--port P | --port-file F) [options]\n";
            std::exit(0);
        } else {
            usage_error("unknown option: " + std::string(arg));
        }
    }

    if (has_port && !args.port_file.empty()) {
        usage_error("use either --port or --port-file, not both");
    }
    if (!has_port && args.port_file.empty()) {
        usage_error("one of --port and --port-file is required");
    }
    if (args.timeout_seconds <= 0) usage_error("--timeout must be positive");

    if (args.has_once) {
        while (!args.once.empty() &&
               (args.once.back() == '\n' || args.once.back() == '\r')) {
            args.once.pop_back();
        }
        if (args.once.empty()) usage_error("--once needs a request");
        if (args.once.find('\n') != std::string::npos ||
            args.once.find('\r') != std::string::npos) {
            usage_error("--once takes exactly one request line");
        }
    } else {
        if (args.conns <= 0) usage_error("-c must be positive");
        if (args.per_conn <= 0) usage_error("-n must be positive");
        if (args.keys <= 0) usage_error("--keys must be positive");
        if (args.pipeline <= 0) usage_error("--pipeline must be positive");
        if (!(args.set_ratio >= 0.0 && args.set_ratio <= 1.0)) {
            usage_error("--set-ratio must be between 0 and 1");
        }
    }
    return args;
}

std::uint16_t resolve_port(const Args& args) {
    if (!args.port_file.empty()) {
        return read_port_file(
            args.port_file.c_str(),
            std::chrono::milliseconds(args.timeout_seconds) * 1000);
    }
    return args.port;
}

sockaddr_in loopback(std::uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return addr;
}

int make_socket() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) kvd_detail::throw_errno("socket");
    return fd;
}

void set_socket_timeout(int fd, int seconds, int option) {
    timeval tv{};
    tv.tv_sec = seconds;
    if (::setsockopt(fd, SOL_SOCKET, option, &tv, sizeof tv) < 0)
        kvd_detail::throw_errno("setsockopt(timeout)");
}

int run_once(const Args& args) {
    const std::uint16_t port = resolve_port(args);
    int fd = make_socket();
    set_socket_timeout(fd, args.timeout_seconds, SO_RCVTIMEO);
    set_socket_timeout(fd, args.timeout_seconds, SO_SNDTIMEO);

    sockaddr_in addr = loopback(port);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        int saved = errno;
        ::close(fd);
        throw std::system_error(saved, std::generic_category(), "connect");
    }

    const std::string request = args.once + "\n";
    if (!write_all(fd, request)) {
        int saved = errno;
        ::close(fd);
        throw std::system_error(saved, std::generic_category(), "write");
    }

    std::string response;
    char buf[512];
    while (response.find('\n') == std::string::npos) {
        ssize_t n = ::recv(fd, buf, sizeof buf, 0);
        if (n > 0) {
            response.append(buf, static_cast<std::size_t>(n));
            if (response.size() > 4096) break;
        } else if (n == 0) {
            break;
        } else if (errno == EINTR) {
            continue;
        } else {
            int saved = errno;
            ::close(fd);
            throw std::system_error(saved, std::generic_category(), "recv");
        }
    }
    ::close(fd);

    if (response.empty()) {
        std::cerr << "kvbench: no response\n";
        return 1;
    }
    if (response.back() == '\n') response.pop_back();
    if (!response.empty() && response.back() == '\r') response.pop_back();
    std::cout << response << '\n';
    return response.starts_with("ERROR") ? 1 : 0;
}

struct Expect {
    enum class Kind { Ok, Missing, Value };
    Kind kind = Kind::Ok;
    std::string value;
};

struct Conn {
    int fd = -1;
    int id = 0;
    bool closed = false;
    std::size_t sent = 0;       // 已经放进输出缓冲的请求数
    std::size_t done = 0;       // 已经校验过的响应数
    std::size_t in_flight = 0;  // 已发但还没收到响应的请求数
    std::uint32_t rng = 1;
    Buffer in;
    Buffer out;
    std::unordered_map<std::string, std::string> values;  // 本地期望值
    std::deque<Expect> pending;
    std::deque<Clock::time_point> sent_at;
};

struct Stats {
    std::size_t requests = 0;
    std::size_t errors = 0;
    std::size_t mismatches = 0;
    std::vector<std::uint64_t> latencies_us;
};

std::uint32_t next_random(std::uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void fill_requests(Conn& conn, const Args& args, std::size_t limit) {
    const std::size_t pipeline = static_cast<std::size_t>(args.pipeline);
    const std::uint32_t key_count = static_cast<std::uint32_t>(args.keys);
    const std::uint32_t threshold =
        static_cast<std::uint32_t>(args.set_ratio * 1000000.0);

    while (conn.in_flight < pipeline && conn.sent < limit) {
        const bool is_set = next_random(conn.rng) % 1000000u < threshold;
        const std::uint32_t key_id = next_random(conn.rng) % key_count;
        // 带上进程号：在同一个服务器上重复跑时，上一轮留下的值不会
        // 让这一轮的 GET 期望值失真。
        const std::string key = "c" + std::to_string(conn.id) + "-p" +
                                std::to_string(::getpid()) + "-k" +
                                std::to_string(key_id);

        std::string line;
        if (is_set) {
            const std::string value =
                "v" + std::to_string(conn.id) + "-" +
                std::to_string(conn.sent);
            // 同一连接上的请求按顺序执行，所以这条 SET 之后再发的 GET
            // 一定看到新值；本地期望表可以立刻更新。
            conn.values[key] = value;
            line = "SET " + key + " " + value + "\n";
            conn.pending.push_back({Expect::Kind::Ok, {}});
        } else {
            auto it = conn.values.find(key);
            if (it == conn.values.end()) {
                line = "GET " + key + "\n";
                conn.pending.push_back({Expect::Kind::Missing, {}});
            } else {
                line = "GET " + key + "\n";
                conn.pending.push_back({Expect::Kind::Value, it->second});
            }
        }
        conn.out.append(line);
        conn.sent_at.push_back(Clock::now());  // 含输出缓冲里的排队时间
        ++conn.sent;
        ++conn.in_flight;
    }
}

bool flush(Conn& conn) {
    while (!conn.out.empty()) {
        ssize_t n = ::send(conn.fd, conn.out.data().data(), conn.out.data().size(),
                           MSG_NOSIGNAL);
        if (n > 0) {
            conn.out.consume(static_cast<std::size_t>(n));
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return true;
        return false;
    }
    return true;
}

void update_events(int epfd, Conn& conn) {
    epoll_event event{};
    event.events = EPOLLIN;
    if (!conn.out.empty()) event.events |= EPOLLOUT;
    event.data.u32 = static_cast<std::uint32_t>(conn.id);
    if (::epoll_ctl(epfd, EPOLL_CTL_MOD, conn.fd, &event) < 0)
        kvd_detail::throw_errno("epoll_ctl");
}

void close_conn(Conn& conn, std::size_t& active) {
    if (conn.closed) return;
    conn.closed = true;
    if (conn.fd >= 0) {
        ::close(conn.fd);
        conn.fd = -1;
    }
    --active;
}

void process_responses(Conn& conn, Stats& stats) {
    for (;;) {
        const std::string_view data = conn.in.data();
        const std::size_t newline = data.find('\n');
        if (newline == std::string_view::npos) break;

        // consume 可能清空底层缓冲，所以先拷贝出这一行再用。
        std::string line(data.substr(0, newline));
        conn.in.consume(newline + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (conn.pending.empty() || conn.sent_at.empty()) {
            ++stats.errors;  // 没发过的请求却来了响应
            continue;
        }

        Expect expect = std::move(conn.pending.front());
        conn.pending.pop_front();
        const Clock::time_point start = conn.sent_at.front();
        conn.sent_at.pop_front();
        --conn.in_flight;
        ++conn.done;
        ++stats.requests;

        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                                 Clock::now() - start)
                                 .count();
        stats.latencies_us.push_back(
            static_cast<std::uint64_t>(elapsed < 0 ? 0 : elapsed));

        bool ok = false;
        switch (expect.kind) {
        case Expect::Kind::Ok:
            ok = line == "OK";
            break;
        case Expect::Kind::Missing:
            ok = line == "NOT_FOUND";
            break;
        case Expect::Kind::Value:
            ok = line == "VALUE " + expect.value;
            break;
        }
        if (!ok) {
            if (line.starts_with("ERROR")) ++stats.errors;
            else ++stats.mismatches;
        }
    }
}

std::uint64_t percentile(const std::vector<std::uint64_t>& sorted, double p) {
    if (sorted.empty()) return 0;
    auto index = static_cast<std::size_t>(
        std::ceil(static_cast<double>(sorted.size()) * p / 100.0));
    if (index > 0) --index;
    if (index >= sorted.size()) index = sorted.size() - 1;
    return sorted[index];
}

int run_bench(const Args& args) {
    const std::uint16_t port = resolve_port(args);
    const std::size_t limit = static_cast<std::size_t>(args.per_conn);

    const int epfd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) kvd_detail::throw_errno("epoll_create1");

    std::vector<Conn> conns(static_cast<std::size_t>(args.conns));
    for (int i = 0; i < args.conns; ++i) {
        const int fd = make_socket();
        sockaddr_in addr = loopback(port);
        // 连接本机服务器是瞬时的；连不上会立刻 ECONNREFUSED。连上之后再
        // 切成非阻塞，事件循环里就不必再处理“连接中”这个状态。
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
            int saved = errno;
            ::close(fd);
            ::close(epfd);
            throw std::system_error(saved, std::generic_category(), "connect");
        }
        set_nonblocking(fd);
        Conn& conn = conns[static_cast<std::size_t>(i)];
        conn.fd = fd;
        conn.id = i;
        conn.rng = 0x9e3779b9u ^ (static_cast<std::uint32_t>(i) + 1) * 0x85ebca6bu;
        if (conn.rng == 0) conn.rng = 1;

        epoll_event event{};
        event.events = EPOLLIN | EPOLLOUT;  // 第一次 EPOLLOUT 时再填请求
        event.data.u32 = static_cast<std::uint32_t>(i);
        if (::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) < 0) {
            int saved = errno;
            ::close(fd);
            ::close(epfd);
            throw std::system_error(saved, std::generic_category(), "epoll_ctl");
        }
    }

    Stats stats;
    const Clock::time_point began = Clock::now();
    const Clock::time_point deadline = began + std::chrono::seconds(args.timeout_seconds);
    std::size_t active = conns.size();
    bool timed_out = false;

    while (active > 0) {
        const Clock::time_point now = Clock::now();
        if (now >= deadline) {
            timed_out = true;
            break;
        }
        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now)
                .count();
        const int wait_ms =
            remaining > std::numeric_limits<int>::max()
                ? std::numeric_limits<int>::max()
                : static_cast<int>(remaining);

        epoll_event events[64];
        const int count = ::epoll_wait(epfd, events, 64, wait_ms);
        if (count < 0) {
            if (errno == EINTR) continue;
            kvd_detail::throw_errno("epoll_wait");
        }
        if (count == 0) continue;

        for (int k = 0; k < count; ++k) {
            Conn& conn = conns[events[k].data.u32];
            if (conn.closed) continue;

            bool failed = false;
            bool eof = false;
            char buf[8192];
            for (;;) {
                const ssize_t n = ::recv(conn.fd, buf, sizeof buf, 0);
                if (n > 0) {
                    conn.in.append(
                        std::string_view(buf, static_cast<std::size_t>(n)));
                } else if (n == 0) {
                    eof = true;
                    break;
                } else if (errno == EINTR) {
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    failed = true;
                    break;
                }
            }
            if (failed) {
                ++stats.errors;
                close_conn(conn, active);
                continue;
            }

            process_responses(conn, stats);
            if (eof && conn.done < limit) {
                ++stats.errors;  // 还没收到全部响应，服务器就先关了
                close_conn(conn, active);
                continue;
            }
            if (conn.done >= limit) {
                close_conn(conn, active);
                continue;
            }

            fill_requests(conn, args, limit);
            if (!flush(conn)) {
                ++stats.errors;
                close_conn(conn, active);
                continue;
            }
            update_events(epfd, conn);
        }
    }

    for (Conn& conn : conns) {
        if (!conn.closed) {
            ::close(conn.fd);
            conn.closed = true;
        }
    }
    ::close(epfd);

    const double seconds =
        std::chrono::duration<double>(Clock::now() - began).count();
    const double throughput =
        seconds > 0.0 ? static_cast<double>(stats.requests) / seconds : 0.0;

    std::vector<std::uint64_t> sorted = stats.latencies_us;
    std::sort(sorted.begin(), sorted.end());
    const std::uint64_t p50 = percentile(sorted, 50.0);
    const std::uint64_t p99 = percentile(sorted, 99.0);

    if (args.csv) {
        std::cout << "conns,requests,seconds,throughput,p50_us,p99_us\n";
        std::cout << args.conns << ',' << stats.requests << ','
                  << std::fixed << std::setprecision(6) << seconds << ','
                  << std::setprecision(1) << throughput << ',' << p50 << ','
                  << p99 << '\n';
    } else {
        std::cout << "requests=" << stats.requests << " errors=" << stats.errors
                  << " mismatches=" << stats.mismatches << '\n';
        if (!args.quiet) {
            std::cout << "throughput=" << std::fixed << std::setprecision(1)
                      << throughput << " req/s p50=" << p50 << " us p99=" << p99
                      << " us\n";
        }
    }

    if (timed_out) {
        std::cerr << "kvbench: timed out after " << args.timeout_seconds << " s\n";
    }
    const std::size_t expected =
        static_cast<std::size_t>(args.conns) * limit;
    return timed_out || stats.errors != 0 || stats.mismatches != 0 ||
                   stats.requests != expected
               ? 1
               : 0;
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);
    try {
        const Args args = parse_args(argc, argv);
        return args.has_once ? run_once(args) : run_bench(args);
    } catch (const std::exception& e) {
        std::cerr << "kvbench: " << e.what() << '\n';
        return 1;
    }
}
