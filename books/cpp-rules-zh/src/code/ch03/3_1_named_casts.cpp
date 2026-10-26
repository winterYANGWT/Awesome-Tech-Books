// 第 3.1 节：四种具名转换各自只做一类事，意图写在代码里
#include <cstdint>
#include <cstdio>
#include <string>

struct Report {
    std::string title;
    int pages;
};

void print(const Report& report) {
    std::printf("%s: %d pages\n", report.title.c_str(), report.pages);
}

struct Device {
    virtual ~Device() = default;
};

struct Camera : Device {
    int megapixels = 24;
};

struct Printer : Device {
    int sheets = 100;
};

void describe(Device& device) {
    // dynamic_cast 查运行期类型：失败时指针版返回 nullptr
    if (auto* camera = dynamic_cast<Camera*>(&device)) {
        std::printf("camera: %d MP\n", camera->megapixels);
    } else {
        std::printf("not a camera\n");
    }
}

int main() {
    double ratio = 7.0 / 2.0;
    // static_cast 处理相关的类型：这里有定义，按值截断
    int pages = static_cast<int>(ratio);
    std::printf("static_cast: %d\n", pages);

    Report draft{"draft", 12};
    const Report& view = draft;
    // const_cast 只改 const 属性；draft 本身不是 const 对象，写回是合法的
    const_cast<Report&>(view).pages = 13;
    print(draft);

    Camera camera;
    Printer printer;
    describe(camera);
    describe(printer);

    const char text[] = "abc";
    // reinterpret_cast 按位重解释指针；这里把 char 看成 unsigned char
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text);
    std::printf("bytes: %u %u %u\n", bytes[0], bytes[1], bytes[2]);

    // 指针 -> 足够大的整数 -> 原指针，是标准保证可逆的少数情形
    std::uintptr_t address = reinterpret_cast<std::uintptr_t>(&draft);
    Report* same = reinterpret_cast<Report*>(address);
    std::printf("reinterpret round trip: %s\n", same == &draft ? "ok" : "bad");
}
