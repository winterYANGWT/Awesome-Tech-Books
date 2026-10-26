// 第 24.6 节：容器存值会切片，元素拷贝太贵就换成智能指针
#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Document {
    std::string title;
    explicit Document(std::string t) : title(std::move(t)) {}
    virtual ~Document() = default;
    virtual std::string kind() const { return "document"; }
};

struct Invoice : Document {
    explicit Invoice(std::string t) : Document(std::move(t)) {}
    std::string kind() const override { return "invoice"; }
};

struct Blob {
    std::array<std::byte, 4096> data{};
    static inline int copies = 0;
    static inline int moves = 0;
    static inline std::size_t bytes_copied = 0;

    Blob() = default;
    Blob(const Blob& other) : data(other.data) {
        ++copies;
        bytes_copied += data.size();
    }
    Blob(Blob&& other) noexcept : data(other.data) { ++moves; }
};

int main() {
    // 存基类值：派生部分被切掉，虚函数也救不回来
    std::vector<Document> shelf;
    shelf.emplace_back("INV-1");       // 这里构造的是 Document
    shelf.push_back(Invoice("INV-2"));  // Invoice 被按 Document 拷贝，切片
    std::cout << "vector<Document> 里的类型: " << shelf[0].kind()
              << "，" << shelf[1].kind() << '\n';

    // 存智能指针：对象仍是 Invoice，多态保留
    std::vector<std::unique_ptr<Document>> archive;
    archive.push_back(std::make_unique<Invoice>("INV-1"));
    archive.push_back(std::make_unique<Invoice>("INV-2"));
    std::cout << "vector<unique_ptr<Document>> 里的类型: " << archive[0]->kind()
              << "，" << archive[1]->kind() << '\n';

    // 拷贝昂贵的元素：容器里放值，每次插入都真的复制整块数据
    Blob prototype;
    Blob::copies = Blob::moves = 0;
    Blob::bytes_copied = 0;
    {
        std::vector<Blob> heavy;
        heavy.reserve(100);
        for (int i = 0; i < 100; ++i) {
            heavy.push_back(prototype);  // 每次都是 4 KiB 的拷贝
        }
    }
    std::cout << "vector<Blob> 插入 100 次: 拷贝 " << Blob::copies
              << " 次，共 " << Blob::bytes_copied << " 字节\n";

    // 同一份数据用 shared_ptr 共享：容器只复制指针
    Blob::copies = Blob::moves = 0;
    Blob::bytes_copied = 0;
    {
        auto shared_blob = std::make_shared<Blob>();
        std::vector<std::shared_ptr<Blob>> light;
        light.reserve(100);
        for (int i = 0; i < 100; ++i) {
            light.push_back(shared_blob);
        }
        std::cout << "vector<shared_ptr<Blob>> 插入 100 次: 拷贝 " << Blob::copies
                  << " 次，引用计数 " << shared_blob.use_count() << '\n';
    }

    // 值语义还意味着赋值和比较都按值工作，不改到容器外的对象
    std::vector<std::string> names{"alpha", "beta"};
    std::string copy = names[0];
    copy[0] = 'A';
    std::cout << "容器内仍是 " << names[0] << "，副本是 " << copy << '\n';
}
