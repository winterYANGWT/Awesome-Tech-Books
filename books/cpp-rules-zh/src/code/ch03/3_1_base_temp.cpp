// 第 3.1 节：static_cast<Base>(*this) 转出的是基类部分的临时副本
#include <cstdio>

struct Report {
    int rendered = 0;

    virtual ~Report() = default;
    virtual void render() { ++rendered; }
    int base_count() const { return rendered; }
};

struct Audit : Report {
    int appended = 0;

    void render() override {
        // 错：转换产生 Report 临时对象，自增的是副本里的 rendered
        static_cast<Report>(*this).render();
        ++appended;
    }

    void render_fixed() {
        // 对：直接在 *this 的基类子对象上调用
        Report::render();
        ++appended;
    }
};

int main() {
    Audit buggy;
    buggy.render();
    std::printf("static_cast: base=%d appended=%d\n",
                buggy.base_count(), buggy.appended);

    Audit fixed;
    fixed.render_fixed();
    std::printf("qualified:   base=%d appended=%d\n",
                fixed.base_count(), fixed.appended);
}
