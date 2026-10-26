// 第 34.4 节：模块可以导出模板声明，把定义留在实现单元
export module catalog;

export namespace store {

template <typename T>
T discounted(T price);

struct Item {
    int cents;
};

}  // namespace store
