// 第 18.3 节：组合只依赖实现的公开接口，可以换实现、放多个实例
#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

template <class Impl>
class NameIndex {
public:
    bool add(const std::string& name) {
        if (contains(name)) {
            return false;
        }
        impl_.push_back(name);
        return true;
    }

    bool contains(const std::string& name) const {
        return std::find(impl_.begin(), impl_.end(), name) != impl_.end();
    }

    std::size_t size() const { return impl_.size(); }

    void print() const {
        for (const auto& name : impl_) {
            std::cout << ' ' << name;
        }
        std::cout << '\n';
    }

private:
    Impl impl_;
};

class IndexBackend {
public:
    virtual ~IndexBackend() = default;
    virtual bool add(const std::string& name) = 0;
    virtual void print() const = 0;
};

class VectorBackend : public IndexBackend {
public:
    bool add(const std::string& name) override { return index_.add(name); }
    void print() const override { index_.print(); }

private:
    NameIndex<std::vector<std::string>> index_;
};

class ListBackend : public IndexBackend {
public:
    bool add(const std::string& name) override { return index_.add(name); }
    void print() const override { index_.print(); }

private:
    NameIndex<std::list<std::string>> index_;
};

int main() {
    NameIndex<std::vector<std::string>> byVector;
    byVector.add("ada");
    byVector.add("bob");
    byVector.add("ada");
    std::cout << "VectorIndex size=" << byVector.size() << "，顺序：";
    byVector.print();

    NameIndex<std::list<std::string>> byList;
    byList.add("bob");
    byList.add("ada");
    std::cout << "ListIndex 顺序：";
    byList.print();

    // 同一个接口，运行期换实现；NameIndex 本身没有继承任何容器。
    std::unique_ptr<IndexBackend> runtime = std::make_unique<VectorBackend>();
    runtime->add("ada");
    runtime->add("bob");
    std::cout << "RuntimeIndex (vector)：";
    runtime->print();
    runtime = std::make_unique<ListBackend>();
    runtime->add("bob");
    runtime->add("ada");
    std::cout << "RuntimeIndex (list)：";
    runtime->print();

    static_assert(!std::is_base_of_v<std::vector<std::string>,
                                     NameIndex<std::vector<std::string>>>);
    static_assert(!std::is_base_of_v<std::list<std::string>,
                                     NameIndex<std::list<std::string>>>);
}
