// 第 26.3 节：指针容器上 remove 只搬移指针，不释放所指对象
#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

struct Session {
    explicit Session(int id) : id(id) { ++alive; }
    ~Session() { --alive; }
    int id;
    static int alive;
};
int Session::alive = 0;

int main() {
    std::vector<Session*> sessions;
    std::vector<Session*> all;  // 只是为了最后统一释放，证明容器没有替我们删
    for (int id : {1, 2, 3, 4, 5, 6}) {
        auto* s = new Session(id);
        sessions.push_back(s);
        all.push_back(s);
    }

    sessions.erase(std::remove_if(sessions.begin(), sessions.end(),
                                  [](Session* s) { return s->id % 2 == 0; }),
                   sessions.end());
    std::printf("裸指针：容器剩 %zu 个，仍存活 %d 个对象\n",
                sessions.size(), Session::alive);
    for (Session* s : all) delete s;  // 不删就泄漏；容器析构不会做这件事
    std::printf("手动释放后存活 %d 个\n", Session::alive);

    // 换成 unique_ptr：remove 搬移智能指针时，被覆盖的旧指针会析构并释放对象
    std::vector<std::unique_ptr<Session>> owned;
    for (int id : {1, 2, 3, 4, 5, 6}) {
        owned.push_back(std::make_unique<Session>(id));
    }
    owned.erase(std::remove_if(owned.begin(), owned.end(),
                               [](const auto& s) { return s->id % 2 == 0; }),
                owned.end());
    std::printf("unique_ptr：容器剩 %zu 个，仍存活 %d 个对象\n",
                owned.size(), Session::alive);
}
