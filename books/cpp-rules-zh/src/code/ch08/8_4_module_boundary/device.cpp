// 第 8.4 节：插件侧负责 Device 的分配和释放
#include "device.h"

#include <iostream>

struct Device {
    int id;
};

extern "C" Device* device_open(int id) {
    std::cout << "plugin: 打开设备 " << id << "\n";
    return new Device{id};
}

extern "C" void device_close(Device* device) {
    std::cout << "plugin: 关闭设备 " << device->id << "\n";
    delete device;
}

extern "C" int device_id(const Device* device) {
    return device->id;
}
