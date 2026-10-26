// 第 24.1 节：按迭代器能力、失效规则和内存布局挑容器，并把选择藏在一个类里
#include <cstdint>
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

// 会报告自己被拷贝、移动、赋值的元素类型，用来看不同容器做同一件事的代价
struct Shipment {
    int id;
    std::string city;
    static inline int copies = 0;
    static inline int moves = 0;
    static inline int assignments = 0;

    Shipment(int i, std::string c) : id(i), city(std::move(c)) {}
    Shipment(const Shipment& other) : id(other.id), city(other.city) { ++copies; }
    Shipment(Shipment&& other) noexcept : id(other.id), city(std::move(other.city)) { ++moves; }
    Shipment& operator=(const Shipment& other) {
        id = other.id;
        city = other.city;
        ++assignments;
        return *this;
    }
    Shipment& operator=(Shipment&& other) noexcept {
        id = other.id;
        city = std::move(other.city);
        ++assignments;
        return *this;
    }
};

// ESTL-02：把容器关在类里，外面只暴露稳定的操作；换容器时影响面只有这个类
class ShipmentTable {
public:
    using Container = std::vector<Shipment>;
    using const_iterator = Container::const_iterator;

    void add(Shipment s) { rows_.push_back(std::move(s)); }
    bool empty() const noexcept { return rows_.empty(); }
    std::size_t size() const noexcept { return rows_.size(); }

    const Shipment* find(int id) const {
        for (const auto& row : rows_) {
            if (row.id == id) {
                return &row;
            }
        }
        return nullptr;
    }

private:
    Container rows_;
};

template <class T>
void report_category(const char* name) {
    using Category = typename std::iterator_traits<typename T::iterator>::iterator_category;
    std::cout << name << " 的迭代器是随机访问: "
              << std::is_same_v<Category, std::random_access_iterator_tag> << '\n';
}

int main() {
    std::cout << std::boolalpha;

    report_category<std::vector<int>>("vector");
    report_category<std::deque<int>>("deque");
    report_category<std::list<int>>("list");

    // C++20 的 contiguous_range 直接问“元素是不是连续存放”
    std::cout << "vector 连续存放: " << std::ranges::contiguous_range<std::vector<int>> << '\n';
    std::cout << "deque 连续存放: " << std::ranges::contiguous_range<std::deque<int>> << '\n';
    std::cout << "list 连续存放: " << std::ranges::contiguous_range<std::list<int>> << '\n';

    // 同样的 push_back，vector 可能搬家，list 的节点不动。
    // 先把 vector 的容量卡到 3，第四次 push_back 一定扩容。
    std::vector<int> numbers;
    numbers.reserve(3);
    numbers.push_back(10);
    numbers.push_back(20);
    numbers.push_back(30);
    const auto vector_before = reinterpret_cast<std::uintptr_t>(&numbers[1]);
    numbers.push_back(40);
    const auto vector_after = reinterpret_cast<std::uintptr_t>(&numbers[1]);
    std::cout << "vector 扩容后原元素地址变了: " << (vector_before != vector_after) << '\n';

    std::list<int> linked{10, 20, 30};
    const auto list_before = reinterpret_cast<std::uintptr_t>(&*std::next(linked.begin()));
    linked.push_back(40);
    const auto list_after = reinterpret_cast<std::uintptr_t>(&*std::next(linked.begin()));
    std::cout << "list 追加后原元素地址变了: " << (list_before != list_after) << '\n';

    // 在中间插入一个元素：list 只挂一个节点，vector 要搬动后半段
    std::vector<Shipment> by_vector;
    by_vector.reserve(8);
    std::list<Shipment> by_list;
    for (int i = 0; i < 6; ++i) {
        by_vector.emplace_back(i, "Shanghai");
        by_list.emplace_back(i, "Shanghai");
    }

    Shipment::copies = Shipment::moves = Shipment::assignments = 0;
    by_vector.insert(by_vector.begin() + 3, Shipment(99, "Beijing"));
    std::cout << "vector 中部插入: 拷贝构造 " << Shipment::copies
              << "，移动构造 " << Shipment::moves
              << "，赋值 " << Shipment::assignments << '\n';

    Shipment::copies = Shipment::moves = Shipment::assignments = 0;
    by_list.insert(std::next(by_list.begin(), 3), Shipment(99, "Beijing"));
    std::cout << "list 中部插入: 拷贝构造 " << Shipment::copies
              << "，移动构造 " << Shipment::moves
              << "，赋值 " << Shipment::assignments << '\n';

    // ESTL-04：判空用 empty()，它不要求容器随时维护精确的元素个数
    ShipmentTable table;
    std::cout << "空表 empty(): " << table.empty() << '\n';
    table.add(Shipment(1, "Hangzhou"));
    std::cout << "加入一条后 empty(): " << table.empty()
              << "，size(): " << table.size() << '\n';
    int raw[3]{};
    std::cout << "std::empty(内置数组): " << std::empty(raw)
              << "，std::size(内置数组): " << std::size(raw) << '\n';

    // 通过类的接口查找，调用方看不到底层是 vector
    if (const Shipment* found = table.find(1)) {
        std::cout << "找到 " << found->id << " 号，目的地 " << found->city << '\n';
    }
}
