// 第 6.7 节的 Pimpl 头文件：只留前置声明和指针，特殊成员只声明不定义
#pragma once

#include <cstddef>
#include <memory>
#include <string>

class Widget {
public:
    Widget();
    explicit Widget(std::string name);
    ~Widget();                                  // 必须声明，定义放到 .cpp

    Widget(const Widget&);
    Widget& operator=(const Widget&);
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;

    void rename(std::string name);
    const std::string& name() const;
    std::size_t size() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
