// 第 11.2 节：delete[] 碰上会抛的析构函数，又发生在栈展开中，只能终止
// mode: crash
// expect: terminate
#include <iostream>
#include <stdexcept>

struct Element {
    int id = 0;

    ~Element() noexcept(false) {          // 故意允许抛出，模拟写坏的析构函数
        std::cout << "  ~Element " << id << "\n";
        if (id == 1) {
            throw std::runtime_error("元素 1 清理失败");
        }
    }
};

struct ArrayHolder {
    Element* array;

    ~ArrayHolder() {                      // 默认 noexcept(true)
        delete[] array;                   // 元素析构抛出，异常从这里逃出
    }
};

void run() {
    ArrayHolder holder{new Element[3]};
    holder.array[0].id = 0;
    holder.array[1].id = 1;
    holder.array[2].id = 2;
    throw std::runtime_error("业务失败");  // 栈展开开始，~ArrayHolder 被调用
}

int main() {
    try {
        run();
    } catch (const std::exception& e) {
        std::cout << "不会到达这里：" << e.what() << "\n";
    }
}
