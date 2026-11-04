// kvd 的协议层：一行一个请求，和 I/O 无关。
//
// 解析器是一个逐字节前进的状态机。这样同一段代码既能处理“一个请求分几次到达”，
// 也能处理“一次到达好几个请求”（流水线）。
//
//   Command --' '--> Key --' '--> Value
//      |              |            |
//      |              |            +--'\n'------> Request（SET）
//      |              +--'\n'-------------------> Request（GET/DEL）
//      |              +--' '------> EndOfLine --'\n'--> Request（GET/DEL）
//      +--'\n'----------------------------------> ParseError
//
//   Value --' '--> EndOfLine（value 里不能有空白，后面只能是行尾）
//   任何非法字节、超长字段、超长行 --> Discard --'\n'--> Command
//
// 超长行必须在读满 1024 字节时就丢掉，不能先攒进内存再判断：对端可以用
// 一条没有换行的长行把服务器的内存撑爆。
#pragma once

#include <cctype>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

struct Request {
    enum class Op { Get, Set, Del };
    Op op = Op::Get;
    std::string key;
    std::string value;
};

struct ParseError {
    std::string reason;  // 例如 "unknown command"、"line too long"
};

// Store 的结果，也是 format_response 的输入。
struct Response {
    enum class Kind { Ok, Value, NotFound, Deleted, Error };
    Kind kind = Kind::Ok;
    std::string data;  // VALUE 的值，或 ERROR 的原因

    static Response ok() { return {}; }
    static Response value(std::string v) { return {Kind::Value, std::move(v)}; }
    static Response not_found() { return {Kind::NotFound, {}}; }
    static Response deleted() { return {Kind::Deleted, {}}; }
    static Response error(std::string reason) {
        return {Kind::Error, std::move(reason)};
    }
};

class Parser {
public:
    using Result = std::variant<Request, ParseError>;

    // 可以喂任意切分的字节；空片段也可以。
    void feed(std::string_view bytes) {
        for (char c : bytes) {
            if (state_ == State::Discard) {
                if (c == '\n') reset_line();
                continue;
            }
            if (c == '\n') {
                saw_cr_ = false;
                finish_line();
                continue;
            }
            if (saw_cr_) {  // '\r' 只能紧挨着 '\n'
                fail("unexpected carriage return");
                continue;
            }
            if (c == '\r') {
                saw_cr_ = true;
                continue;
            }
            if (++line_len_ > kMaxLine) {
                fail("line too long");
                continue;
            }
            if (c == ' ') {
                on_space();
                continue;
            }
            if (std::isspace(static_cast<unsigned char>(c))) {
                fail("unexpected whitespace");
                continue;
            }
            on_char(c);
        }
    }

    // 取出一个完整结果；还没有完整的行时返回空。
    std::optional<Result> next() {
        if (ready_.empty()) return std::nullopt;
        Result r = std::move(ready_.front());
        ready_.pop_front();
        return r;
    }

private:
    enum class State { Command, Key, Value, EndOfLine, Discard };

    static constexpr std::size_t kMaxLine = 1024;
    static constexpr std::size_t kMaxField = 256;

    void on_char(char c) {
        switch (state_) {
        case State::Command:
            token_.push_back(c);
            break;
        case State::Key:
            token_.push_back(c);
            if (token_.size() > kMaxField) fail("key too long");
            break;
        case State::Value:
            token_.push_back(c);
            if (token_.size() > kMaxField) fail("value too long");
            break;
        case State::EndOfLine:
            fail("unexpected token");
            break;
        case State::Discard:
            break;
        }
    }

    void on_space() {
        switch (state_) {
        case State::Command:
            if (token_.empty()) {
                fail("empty command");
            } else if (token_ == "SET") {
                op_ = Request::Op::Set;
                state_ = State::Key;
                token_.clear();
            } else if (token_ == "GET" || token_ == "DEL") {
                op_ = token_ == "GET" ? Request::Op::Get : Request::Op::Del;
                state_ = State::Key;
                token_.clear();
            } else {
                fail("unknown command");
            }
            break;
        case State::Key:
            if (token_.empty()) {
                fail("empty key");
            } else {
                key_ = std::move(token_);
                token_.clear();
                state_ = op_ == Request::Op::Set ? State::Value : State::EndOfLine;
            }
            break;
        case State::Value:
            if (token_.empty()) {
                fail("empty value");
            } else {
                // value 已经读完；后面多出来的东西由 EndOfLine 拒绝。
                state_ = State::EndOfLine;
            }
            break;
        case State::EndOfLine:
            break;  // 行尾的空白无害
        case State::Discard:
            break;
        }
    }

    void finish_line() {
        switch (state_) {
        case State::Command:
            emit_error(token_.empty() ? "empty command" : "missing key");
            break;
        case State::Key:
            if (token_.empty()) {
                emit_error("empty key");
            } else if (op_ == Request::Op::Set) {
                emit_error("missing value");
            } else {
                key_ = std::move(token_);
                push_request();
            }
            break;
        case State::Value:
            if (token_.empty()) emit_error("empty value");
            else push_request();
            break;
        case State::EndOfLine:
            push_request();
            break;
        case State::Discard:
            reset_line();
            break;
        }
    }

    void push_request() {
        Request r;
        r.op = op_;
        r.key = std::move(key_);
        if (op_ == Request::Op::Set) r.value = std::move(token_);
        ready_.push_back(std::move(r));
        reset_line();
    }

    void emit_error(std::string reason) {
        ready_.push_back(ParseError{std::move(reason)});
        reset_line();
    }

    // 出错后不立刻丢掉缓冲区：先记下错误，再跳到下一个 '\n'。
    void fail(std::string reason) {
        ready_.push_back(ParseError{std::move(reason)});
        token_.clear();
        key_.clear();
        saw_cr_ = false;
        line_len_ = 0;
        state_ = State::Discard;
    }

    void reset_line() {
        state_ = State::Command;
        token_.clear();
        key_.clear();
        saw_cr_ = false;
        line_len_ = 0;
    }

    State state_ = State::Command;
    Request::Op op_ = Request::Op::Get;
    std::string token_;  // 正在读的字段
    std::string key_;    // 已经读完的 key
    std::size_t line_len_ = 0;
    bool saw_cr_ = false;
    std::deque<Result> ready_;
};

inline std::string format_response(const Response& r) {
    switch (r.kind) {
    case Response::Kind::Ok:
        return "OK\n";
    case Response::Kind::Value:
        return "VALUE " + r.data + "\n";
    case Response::Kind::NotFound:
        return "NOT_FOUND\n";
    case Response::Kind::Deleted:
        return "DELETED\n";
    case Response::Kind::Error:
        return "ERROR " + r.data + "\n";
    }
    return "ERROR internal\n";  // 所有枚举都处理了，这里只是让编译器满意
}

inline std::string format_response(const ParseError& e) {
    return "ERROR " + e.reason + "\n";
}
