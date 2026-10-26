// 第 2.7 节：反例：double 到 float 的窄化在拷贝初始化里是静默的
// mode: warn
// flags: -Wconversion
// expect: conversion
#include <iostream>

int main() {
    double d = 100000001.0;
    float f = d;   // 1e8 附近相邻 float 的间隔是 8，这个值装不下
    std::cout << f << '\n';
}
