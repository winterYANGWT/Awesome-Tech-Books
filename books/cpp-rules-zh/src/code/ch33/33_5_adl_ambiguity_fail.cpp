// 第 33.5 节：ADL 把另一个命名空间的同名函数也拉进重载集
// mode: fail
// expect: ambiguous
namespace ui {
struct Widget {
    int id;
};

void draw(Widget widget) { (void)widget; }
}

namespace render {
void draw(ui::Widget widget) { (void)widget; }

void run(ui::Widget widget) {
    draw(widget);   // render::draw 与 ADL 找到的 ui::draw 一样好
}
}

int main() {
    render::run({1});
}
