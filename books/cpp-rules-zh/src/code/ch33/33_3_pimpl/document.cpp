// 第 33.3 节：Impl 在 .cpp 里完整定义，客户端永远看不到
#include "document.h"

namespace doc {
struct Document::Impl {
    std::string body;
    mutable int reads = 0;   // 私有数据，随便改，客户端不用重编译
};

Document::Document(std::string title) : impl_(std::make_unique<Impl>()) {
    impl_->body = std::move(title);
}

// 定义点能看到完整的 Impl，unique_ptr 的默认删除器才能通过 static_assert。
Document::~Document() = default;
Document::Document(Document&&) noexcept = default;
Document& Document::operator=(Document&&) noexcept = default;

const std::string& Document::title() const { return impl_->body; }

std::string Document::render() const {
    ++impl_->reads;
    return impl_->body;
}

std::string HtmlDocument::render() const {
    return "<p>" + Document::render() + "</p>";
}
}
