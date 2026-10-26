// 第 13.2 节：翻译时补充高层语义，用 nested_exception 保留低层原因
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

struct UnexpectedEof : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct DocumentCorrupt : std::runtime_error, std::nested_exception {
    using std::runtime_error::runtime_error;
};

void parse_body(bool truncated) {
    if (truncated) {
        throw UnexpectedEof("unexpected end of file at line 12");
    }
}

void open_document(bool truncated) {
    try {
        parse_body(truncated);
    } catch (const UnexpectedEof& e) {
        // 低层错误说的是“文件提前结束”，上层知道这意味着文档损坏。
        // 翻译成更高层的异常，同时把原异常挂在 nested_exception 里。
        throw_with_nested(DocumentCorrupt("document is corrupt: truncated body"));
    }
}

void print_chain(const std::exception& error, int depth = 0) {
    std::cout << std::string(static_cast<std::size_t>(depth) * 2, ' ')
              << error.what() << "\n";
    if (const auto* nested = dynamic_cast<const std::nested_exception*>(&error)) {
        try {
            nested->rethrow_nested();
        } catch (const std::exception& inner) {
            print_chain(inner, depth + 1);
        }
    }
}

int main() {
    try {
        open_document(true);
    } catch (const std::exception& e) {
        std::cout << "顶层捕获，异常链：\n";
        print_chain(e);
    }

    try {
        open_document(false);
        std::cout << "完整文档解析成功\n";
    } catch (const std::exception& e) {
        std::cout << "不应到达这里：" << e.what() << "\n";
    }
}
