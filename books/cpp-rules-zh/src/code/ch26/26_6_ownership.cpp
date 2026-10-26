// 第 26.6 节：容器析构只销毁元素本身，裸指针不会 delete 所指对象
#include <cstdio>
#include <memory>
#include <vector>

struct Resource {
    explicit Resource(int id) : id(id) {
        ++alive;
        std::printf("  打开资源 %d\n", id);
    }
    ~Resource() {
        --alive;
        std::printf("  关闭资源 %d\n", id);
    }
    int id;
    static int alive;
};
int Resource::alive = 0;

int main() {
    std::vector<Resource*> raw;
    {
        raw.push_back(new Resource(1));
        raw.push_back(new Resource(2));
    }  // 作用域结束，只销毁两个指针
    std::printf("裸指针容器销毁后，仍存活 %d 个对象\n", Resource::alive);

    for (Resource* p : raw) delete p;  // 必须自己释放
    std::printf("手动 delete 后，存活 %d 个对象\n", Resource::alive);

    {
        std::vector<std::unique_ptr<Resource>> owned;
        owned.push_back(std::make_unique<Resource>(3));
        owned.push_back(std::make_unique<Resource>(4));
        std::printf("unique_ptr 容器里 %zu 个元素\n", owned.size());
    }  // 容器析构，unique_ptr 析构，对象自动释放
    std::printf("unique_ptr 容器销毁后，存活 %d 个对象\n", Resource::alive);
}
