// 第 18.4 节：两个基类有同名函数时，即使一个不可访问，名字查找仍然二义
// mode: fail
// expect: ambiguous
class BorrowableItem {
public:
    void checkOut() {}
};

class ElectronicGadget {
private:
    bool checkOut() const { return true; }
};

class MP3Player : public BorrowableItem, public ElectronicGadget {};

int main() {
    MP3Player player;
    player.checkOut();  // 二义：哪一个 checkOut？
}
