#pragma once

// 调用方只见前置声明，看不到 Device 的大小和布局。
struct Device;

extern "C" Device* device_open(int id);
extern "C" void device_close(Device* device);
extern "C" int device_id(const Device* device);
