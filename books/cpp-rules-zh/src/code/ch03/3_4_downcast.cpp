// 第 3.4 节：dynamic_cast 查运行期类型，引用版失败抛 bad_cast
#include <cstdio>
#include <typeinfo>

struct Device {
    virtual ~Device() = default;
    virtual const char* kind() const { return "device"; }
};

struct Networked {
    virtual ~Networked() = default;
    virtual int link() const { return 0; }
};

struct Camera : Device, Networked {
    const char* kind() const override { return "camera"; }
    int link() const override { return 100; }
    int megapixels = 24;
};

struct Printer : Device {
    const char* kind() const override { return "printer"; }
    int sheets = 100;
};

void inspect(Device& device) {
    if (auto* camera = dynamic_cast<Camera*>(&device)) {
        std::printf("pointer: camera %d MP\n", camera->megapixels);
    } else {
        std::printf("pointer: not camera (%s)\n", device.kind());
    }

    try {
        Camera& camera = dynamic_cast<Camera&>(device);
        std::printf("reference: camera %d MP\n", camera.megapixels);
    } catch (const std::bad_cast&) {
        std::printf("reference: bad_cast\n");
    }

    // 交叉转换：沿继承图从 Device 一侧转到 Networked 一侧
    if (auto* net = dynamic_cast<Networked*>(&device)) {
        std::printf("network: link %d\n", net->link());
    } else {
        std::printf("network: not networked\n");
    }
}

int main() {
    Camera camera;
    Printer printer;
    inspect(camera);
    inspect(printer);

    // 已经确知动态类型时，static_cast 是合法的，也没有运行期开销
    Camera& known = static_cast<Camera&>(camera);
    std::printf("known camera: %d MP\n", known.megapixels);
}
