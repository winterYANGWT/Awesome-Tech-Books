// 第 33.3 节：Pimpl 头文件只留可见接口、protected 成员和不透明指针
#ifndef CH33_DOCUMENT_H
#define CH33_DOCUMENT_H

#include <memory>
#include <string>

namespace doc {
class Document {
public:
    explicit Document(std::string title);
    virtual ~Document();                       // 定义必须放到 .cpp

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;

    const std::string& title() const;
    virtual std::string render() const;        // 虚函数必须留在可见类

protected:
    int revision_ = 0;                         // protected 成员不能藏进 Impl

private:
    struct Impl;                               // 只声明，不定义
    std::unique_ptr<Impl> impl_;
};

class HtmlDocument : public Document {
public:
    using Document::Document;
    std::string render() const override;
    void revise() { ++revision_; }             // 派生类要用 protected 成员
};
}
#endif
