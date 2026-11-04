#include "protocol.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

int failures = 0;

std::string render(const Parser::Result& result) {
    if (auto* request = std::get_if<Request>(&result)) {
        const char* op = request->op == Request::Op::Get   ? "GET"
                         : request->op == Request::Op::Set ? "SET"
                                                           : "DEL";
        return std::string("request ") + op + " [" + request->key + "] [" +
               request->value + "]";
    }
    return "error " + std::get<ParseError>(result).reason;
}

std::vector<std::string> drain(Parser& parser) {
    std::vector<std::string> out;
    while (auto result = parser.next()) out.push_back(render(*result));
    return out;
}

std::vector<std::string> parse_all(std::string_view input) {
    Parser parser;
    parser.feed(input);
    return drain(parser);
}

// 在所有可能的位置切成两段，每段喂完都先取一次结果。
std::vector<std::string> parse_split(std::string_view input, std::size_t at) {
    Parser parser;
    parser.feed(input.substr(0, at));
    std::vector<std::string> out = drain(parser);
    parser.feed(input.substr(at));
    std::vector<std::string> more = drain(parser);
    out.insert(out.end(), more.begin(), more.end());
    return out;
}

// 随机切成 1..9 字节的小段，覆盖“两段”之外的分法。
std::vector<std::string> parse_random(std::string_view input, std::uint32_t seed) {
    std::mt19937 rng(seed);
    Parser parser;
    std::vector<std::string> out;
    std::size_t pos = 0;
    while (pos < input.size()) {
        const std::size_t step =
            std::min<std::size_t>(1 + rng() % 9, input.size() - pos);
        parser.feed(input.substr(pos, step));
        std::vector<std::string> more = drain(parser);
        out.insert(out.end(), more.begin(), more.end());
        pos += step;
    }
    return out;
}

void check_equal(const std::vector<std::string>& got,
                 const std::vector<std::string>& want, const std::string& what) {
    if (got == want) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n  want:";
    for (const auto& line : want) std::cerr << "\n    " << line;
    std::cerr << "\n  got:";
    for (const auto& line : got) std::cerr << "\n    " << line;
    std::cerr << '\n';
}

void check_parse(std::string_view input,
                 const std::vector<std::string>& want) {
    const std::string name = "parse";
    check_equal(parse_all(input), want, name);
    for (std::size_t at = 0; at <= input.size(); ++at) {
        check_equal(parse_split(input, at), want,
                    name + " split at " + std::to_string(at));
    }
    for (std::uint32_t seed = 0; seed < 50; ++seed) {
        check_equal(parse_random(input, seed), want,
                    name + " random seed " + std::to_string(seed));
    }
}

void check(bool ok, std::string_view what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << '\n';
}

void test_requests() {
    check_parse("SET name alice\n",
                {"request SET [name] [alice]"});
    check_parse("GET name\n", {"request GET [name] []"});
    check_parse("DEL name\n", {"request DEL [name] []"});
    check_parse("SET k v\r\n", {"request SET [k] [v]"});
    check_parse("SET a 1\nGET a\nDEL a\nGET a\n",
                {"request SET [a] [1]", "request GET [a] []",
                 "request DEL [a] []", "request GET [a] []"});
}

void test_errors() {
    check_parse("\n", {"error empty command"});
    check_parse("BOGUS x\n", {"error unknown command"});
    check_parse("GET\n", {"error missing key"});
    check_parse("GET \n", {"error empty key"});
    check_parse("SET k\n", {"error missing value"});
    check_parse("SET k \n", {"error empty value"});
    check_parse("SET k  \n", {"error empty value"});
    check_parse("GET k extra\n", {"error unexpected token"});
    check_parse("SET k v extra\n", {"error unexpected token"});
    check_parse("GET k\tv\n", {"error unexpected whitespace"});
    check_parse("GET k\rX\n", {"error unexpected carriage return"});
    check_parse(std::string("GET ") + std::string(257, 'k') + "\n",
                {"error key too long"});
    check_parse(std::string("SET k ") + std::string(257, 'v') + "\n",
                {"error value too long"});

    // 出错后只丢到这一行的 '\n'，下一行的请求照常解析。
    check_parse("GET k extra\nSET b 2\n",
                {"error unexpected token", "request SET [b] [2]"});
    check_parse(std::string(2000, 'x') + "\nGET b\n",
                {"error line too long", "request GET [b] []"});
}

void test_line_limit() {
    Parser parser;
    parser.feed(std::string(1024, 'x'));  // 正好 1024 字节，还不算超长
    check(!parser.next().has_value(), "1024 bytes without newline");
    parser.feed("\n");
    auto result = parser.next();
    check(result.has_value(), "1024-byte line is complete at newline");
    if (result) check(std::get<ParseError>(*result).reason == "missing key",
                      "1024-byte command has no key");

    Parser overlong;
    overlong.feed(std::string(1025, 'x'));
    auto error = overlong.next();
    check(error.has_value(), "1025-byte line reports at once");
    if (error) check(std::get<ParseError>(*error).reason == "line too long",
                     "overlong reason");
    overlong.feed("\nGET a\n");
    auto next = overlong.next();
    check(next && std::holds_alternative<Request>(*next),
          "recovers after the overlong line");
}

void test_incomplete_and_empty_feed() {
    Parser parser;
    parser.feed("");
    check(!parser.next().has_value(), "empty feed");
    parser.feed("GET a");
    check(!parser.next().has_value(), "no result before newline");
    parser.feed("\n");
    auto result = parser.next();
    check(result.has_value(), "result after newline");
    if (result) check(std::holds_alternative<Request>(*result), "GET is a request");
}

void test_format_response() {
    check(format_response(Response::ok()) == "OK\n", "format OK");
    check(format_response(Response::value("v")) == "VALUE v\n", "format VALUE");
    check(format_response(Response::not_found()) == "NOT_FOUND\n",
          "format NOT_FOUND");
    check(format_response(Response::deleted()) == "DELETED\n", "format DELETED");
    check(format_response(Response::error("bad")) == "ERROR bad\n",
          "format ERROR");
    check(format_response(ParseError{"line too long"}) ==
              "ERROR line too long\n",
          "format ParseError");
}

}  // namespace

int main() {
    test_requests();
    test_errors();
    test_line_limit();
    test_incomplete_and_empty_feed();
    test_format_response();

    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "test_protocol: OK\n";
    return 0;
}
