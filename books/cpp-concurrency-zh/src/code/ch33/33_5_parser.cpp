// mode: run
// 同一批请求按 1 字节、3 字节、整块三种切分喂给同一个状态机。
#include <cctype>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct Request {
    enum class Op { Get, Set, Del };
    Op op = Op::Get;
    std::string key;
    std::string value;
};

struct ParseError {
    std::string reason;
};

class Parser {
public:
    using Result = std::variant<Request, ParseError>;

    // ANCHOR: feed
    void feed(std::string_view bytes) {
        for (char c : bytes) {
            if (state_ == State::Discard) {
                if (c == '\n') reset_line();
                continue;
            }
            if (c == '\n') {
                finish_line();
                reset_line();
                continue;
            }
            if (c == '\r') continue;
            if (++line_len_ > kMaxLine) {
                fail("line too long");
                continue;
            }
            if (c == ' ') on_space();
            else on_char(c);
        }
    }

    // ANCHOR_END: feed

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
        token_.push_back(c);
        if (state_ == State::Key && token_.size() > kMaxField) fail("key too long");
        if (state_ == State::Value && token_.size() > kMaxField) fail("value too long");
        if (state_ == State::EndOfLine) fail("unexpected token");
    }

    void on_space() {
        if (state_ == State::Command) {
            if (token_ == "SET") { op_ = Request::Op::Set; state_ = State::Key; command_seen_ = true; }
            else if (token_ == "GET" || token_ == "DEL") {
                op_ = token_ == "GET" ? Request::Op::Get : Request::Op::Del;
                state_ = State::Key;
                command_seen_ = true;
            } else { fail("unknown command"); return; }
            token_.clear();
        } else if (state_ == State::Key) {
            if (token_.empty()) { fail("empty key"); return; }
            key_ = std::move(token_);
            token_.clear();
            state_ = op_ == Request::Op::Set ? State::Value : State::EndOfLine;
        } else if (state_ == State::Value) {
            if (token_.empty()) { fail("empty value"); return; }
            state_ = State::EndOfLine;
        }
    }

    void finish_line() {
        if (state_ == State::Key && !token_.empty() && op_ != Request::Op::Set && command_seen_) {
            key_ = std::move(token_);
            push_request();
        } else if (state_ == State::Value && !token_.empty()) {
            push_request();
        } else if (state_ == State::EndOfLine) {
            push_request();
        } else {
            fail("incomplete request");
        }
        token_.clear();
        key_.clear();
    }

    void push_request() {
        Request r;
        r.op = op_;
        r.key = key_;
        if (op_ == Request::Op::Set) r.value = token_;
        ready_.push_back(std::move(r));
    }

    void fail(std::string reason) {
        ready_.push_back(ParseError{std::move(reason)});
        token_.clear();
        key_.clear();
        line_len_ = 0;
        state_ = State::Discard;
    }

    void reset_line() {
        state_ = State::Command;
        token_.clear();
        key_.clear();
        line_len_ = 0;
    }

    State state_ = State::Command;
    Request::Op op_ = Request::Op::Get;
    std::string token_;
    std::string key_;
    std::size_t line_len_ = 0;
    bool command_seen_ = false;
    std::deque<Result> ready_;
};

static std::string format(const Parser::Result &r) {
    if (auto *e = std::get_if<ParseError>(&r)) return "ERROR " + e->reason;
    const Request &q = std::get<Request>(r);
    if (q.op == Request::Op::Set) return "SET " + q.key + " " + q.value;
    if (q.op == Request::Op::Get) return "GET " + q.key;
    return "DEL " + q.key;
}

static std::vector<std::string> run(std::string_view input, std::size_t chunk) {
    Parser parser;
    std::vector<std::string> out;
    for (std::size_t i = 0; i < input.size(); i += chunk) {
        parser.feed(input.substr(i, chunk));
        while (auto r = parser.next()) out.push_back(format(*r));
    }
    return out;
}

int main() {
    const std::string input =
        "SET alpha 1\nGET alpha\nDEL alpha\nGET missing\nSET beta 2\n";
    const auto one = run(input, 1);
    const auto three = run(input, 3);
    const auto whole = run(input, input.size());

    auto show = [](const char *name, const std::vector<std::string> &v) {
        std::printf("%s:", name);
        for (const auto &s : v) std::printf(" [%s]", s.c_str());
        std::printf("\n");
    };
    show("split=1", one);
    show("split=3", three);
    show("whole ", whole);
    std::printf("identical: %s\n", (one == three && one == whole) ? "yes" : "no");
    return 0;
}
