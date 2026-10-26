// 第 33.3 节：客户端只看到 document.h
#include "document.h"

#include <iostream>
#include <type_traits>

int main() {
    doc::HtmlDocument page("release notes");
    page.revise();
    page.revise();

    std::cout << "render=" << page.render() << '\n';
    std::cout << "sizeof(Document)=" << sizeof(doc::Document) << '\n';
    std::cout << "copyable="
              << (std::is_copy_constructible_v<doc::Document> ? "yes" : "no") << '\n';
}
