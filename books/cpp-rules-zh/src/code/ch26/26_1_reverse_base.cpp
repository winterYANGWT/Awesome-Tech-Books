// 第 26.1 节：reverse_iterator 的 base() 指向所指数值的下一个位置
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <vector>

void print(const char* label, const std::vector<int>& v) {
    std::printf("%s:", label);
    for (int x : v) std::printf(" %d", x);
    std::puts("");
}

int main() {
    const std::vector<int> original{10, 20, 30, 40, 50};

    {
        // 插入：在反向遍历中“ri 的位置”插入，就是往 base() 处插
        std::vector<int> v = original;
        auto ri = std::find(v.rbegin(), v.rend(), 30);
        std::printf("ri 指向 %d，base() 指向 %d\n", *ri, *ri.base());
        v.insert(ri.base(), 99);
        print("insert(ri.base(), 99)", v);
    }

    {
        // 删除：ri.base() 指向 40，直接删它删掉的是 40
        std::vector<int> v = original;
        auto ri = std::find(v.rbegin(), v.rend(), 30);
        v.erase(ri.base());
        print("erase(ri.base())", v);
    }

    {
        // 删除 ri 所指元素：先让 ri 走到 40，再取 base()，落在 30 上
        std::vector<int> v = original;
        auto ri = std::find(v.rbegin(), v.rend(), 30);
        v.erase((++ri).base());
        print("erase((++ri).base())", v);
    }

    {
        // 本机 libstdc++ 的 vector 迭代器是包装类，--ri.base() 能编译；
        // 原书说 vector/string 上常编译不过，那是迭代器直接就是指针的实现。
        std::vector<int> v = original;
        auto ri = std::find(v.rbegin(), v.rend(), 30);
        v.erase(--ri.base());
        print("erase(--ri.base())", v);
    }

    {
        // 不依赖实现的写法：std::prev 先复制 base()，再减一
        std::vector<int> v = original;
        auto ri = std::find(v.rbegin(), v.rend(), 30);
        v.erase(std::prev(ri.base()));
        print("erase(std::prev(ri.base()))", v);
    }
}
