// 第 17.3 节：纯虚、普通虚、非虚各自继承什么
#include <iostream>
#include <string>
#include <type_traits>

class Document {
public:
    explicit Document(std::string title) : title_(std::move(title)) {}
    virtual ~Document() = default;

    // 纯虚：只给接口。每个具体文档都必须自己回答“正文是什么”。
    virtual std::string body() const = 0;

    // 普通虚：接口 + 默认实现。派生类不写，就默默继承这个默认行为。
    virtual std::string footer() const { return "(no footer)"; }

    // 非虚：接口 + 强制实现。所有派生类共用同一套标题格式。
    std::string header() const { return "== " + title_ + " =="; }

    // ESS-09：共享实现可以上提，抽象基类不必是纯接口。
    std::string decorated() const {
        return header() + "\n" + body() + "\n" + footer();
    }

private:
    std::string title_;
};

class Invoice : public Document {
public:
    using Document::Document;
    std::string body() const override { return "invoice body"; }
    std::string footer() const override { return "thank you"; }
};

class Memo : public Document {
public:
    using Document::Document;
    std::string body() const override { return "memo body"; }
    // 故意不覆盖 footer：看默认实现会怎样被静默继承。
};

int main() {
    static_assert(std::is_abstract_v<Document>);

    Invoice invoice("Invoice #42");
    Memo memo("Memo");

    std::cout << invoice.decorated() << "\n--\n";
    std::cout << memo.decorated() << "\n";
}
