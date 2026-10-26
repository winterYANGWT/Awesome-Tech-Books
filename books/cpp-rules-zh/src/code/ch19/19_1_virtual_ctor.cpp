// 第 19.1 节：构造函数不能虚，但工厂函数和 clone 能按动态类型创建、复制对象
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

class Document {
public:
    virtual ~Document() = default;
    virtual std::string kind() const = 0;
    virtual std::string body() const = 0;
    virtual void print(std::ostream& os) const {
        os << kind() << ": " << body();
    }

    // 虚拷贝构造：返回类型用基类智能指针，派生类覆盖时保持同一类型。
    virtual std::unique_ptr<Document> clone() const = 0;
};

class Report : public Document {
public:
    explicit Report(std::string body) : body_(std::move(body)) {}

    std::string kind() const override { return "report"; }
    std::string body() const override { return body_; }

    std::unique_ptr<Document> clone() const override {
        std::cout << "  clone report\n";
        return std::make_unique<Report>(*this);
    }

private:
    std::string body_;
};

class Invoice : public Document {
public:
    explicit Invoice(int amount) : amount_(amount) {}

    std::string kind() const override { return "invoice"; }
    std::string body() const override { return "amount=" + std::to_string(amount_); }

    std::unique_ptr<Document> clone() const override {
        std::cout << "  clone invoice\n";
        return std::make_unique<Invoice>(*this);
    }

private:
    int amount_;
};

// 非成员 operator<< 不能是虚函数，转调虚成员 print 就获得了动态行为。
std::ostream& operator<<(std::ostream& os, const Document& doc) {
    doc.print(os);
    return os;
}

// “虚构造函数”：普通函数根据输入决定 new 哪个派生类。
std::unique_ptr<Document> read_document(std::string_view tag, int amount) {
    if (tag == "report") {
        return std::make_unique<Report>("quarterly numbers");
    }
    if (tag == "invoice") {
        return std::make_unique<Invoice>(amount);
    }
    return nullptr;
}

int main() {
    static_assert(std::is_abstract_v<Document>);
    static_assert(std::is_polymorphic_v<Document>);

    std::vector<std::unique_ptr<Document>> originals;
    originals.push_back(read_document("report", 0));
    originals.push_back(read_document("invoice", 42));
    originals.push_back(read_document("report", 0));

    std::vector<std::unique_ptr<Document>> copies;
    for (const auto& doc : originals) {
        copies.push_back(doc->clone());
    }

    std::cout << "原对象：\n";
    for (const auto& doc : originals) {
        std::cout << "  " << *doc << '\n';
    }
    std::cout << "克隆对象：\n";
    for (const auto& doc : copies) {
        std::cout << "  " << *doc << '\n';
    }
}
