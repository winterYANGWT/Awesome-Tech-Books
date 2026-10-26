// 第 8.4 节：跨共享库边界，由分配方提供释放函数
#include "device.h"

#include <iostream>
#include <memory>

int main() {
    {
        // unique_ptr 把删除器写进类型；这里删除器是 plugin 导出的函数。
        std::unique_ptr<Device, void (*)(Device*)> one(device_open(1),
                                                       &device_close);
        std::cout << "main: 读取设备 " << device_id(one.get()) << "\n";
    }

    {
        // shared_ptr 把删除器存进控制块，拷贝出去的 shared_ptr 也用它。
        std::shared_ptr<Device> two(device_open(2), &device_close);
        auto copy = two;
        std::cout << "main: 引用计数 " << two.use_count() << "\n";
    }

    std::cout << "main: 两个对象都由 plugin 的 device_close 释放\n";
}
