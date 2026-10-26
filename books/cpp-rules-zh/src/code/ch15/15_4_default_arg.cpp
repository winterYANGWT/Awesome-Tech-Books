// 第 15.4 节：默认实参应当是常用值，不是选择另一条代码路径的开关
#include <cstddef>
#include <iostream>
#include <string>

enum class Mode { streaming, capped };

struct Logger {
    std::string path;
    Mode mode;
    std::size_t limit;
};

// 旧接口：max_size == 0 被当成“不设上限”的哨兵，函数体分成两条路。
Logger open_log(const std::string& path, std::size_t max_size = 0) {
    if (max_size == 0) {
        return Logger{path, Mode::streaming, 0};
    }
    return Logger{path, Mode::capped, max_size};
}

// 新接口：两条路径各有名字，调用点一眼看出选的是哪条。
Logger open_stream(const std::string& path) {
    return Logger{path, Mode::streaming, 0};
}

Logger open_capped(const std::string& path, std::size_t max_size) {
    return Logger{path, Mode::capped, max_size};
}

// 好默认值：颜色是普通参数，两种调用走同一段代码。
enum class Color { black, red };

void draw_box(int width, int height, Color color = Color::black) {
    std::cout << "draw_box " << width << 'x' << height
              << " color=" << (color == Color::black ? "black" : "red") << '\n';
}

void describe(const Logger& log) {
    std::cout << log.path << ": "
              << (log.mode == Mode::streaming ? "streaming" : "capped")
              << " limit=" << log.limit << '\n';
}

int main() {
    describe(open_log("app.log"));        // 默认值走 streaming 分支
    describe(open_log("app.log", 0));     // 显式写 0，还是 streaming，意图看不出来
    describe(open_log("app.log", 4096));  // capped
    describe(open_stream("app.log"));
    describe(open_capped("app.log", 4096));

    draw_box(3, 2);
    draw_box(3, 2, Color::red);
}
