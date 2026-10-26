// 第 9.4 节：new[] 配了单个对象的 delete，数组元素不会逐个析构
// mode: crash
// expect: AddressSanitizer
#include <iostream>
#include <memory>

struct Packet {
    Packet() : id(0) {}
    explicit Packet(int n) : id(n) {}
    ~Packet() { std::cout << "  析构 Packet " << id << "\n"; }
    int id;
};

int main() {
    std::cout << "把 new Packet[3] 交给只调用 delete 的 unique_ptr<Packet>\n";
    {
        std::unique_ptr<Packet> wrong(new Packet[3]);
        std::cout << "  第一个元素 id = " << wrong->id << "\n";
    }
    std::cout << "如果还能走到这里，说明没有触发检查\n";
}
