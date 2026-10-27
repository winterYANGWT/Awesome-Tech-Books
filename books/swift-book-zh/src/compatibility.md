# 版本兼容性 {#Version-Compatibility}

了解在较早的语言模式下可以使用哪些功能。

本书介绍 Swift 6.4，它是 Xcode 27 中自带的默认 Swift 版本。你可以使用 Swift 6.4 编译器来构建分别用 Swift 6.4、Swift 5、Swift 4.2 或 Swift 4 编写的代码。

当你使用 Swift 6.4 编译器构建使用 Swift 5 语言模式的代码时，你可以使用 Swift 6.4 的新功能——它们要么默认启用，要么通过 upcoming feature 标志启用。不过，要启用严格并发检查，你需要升级到 Swift 6.4 语言模式。

此外，当你使用 Xcode 15.3 构建 Swift 4 和 Swift 4.2 代码时，大部分 Swift 5 功能仍然可用。不过，以下变更仅适用于使用 Swift 5 或更高版本语言模式的代码：

- 返回不透明类型的函数需要 Swift 5.1 运行时。
- `try?` 表达式不会给已经返回可选值的表达式引入额外的可选层级。
- 大整数字面量的初始化表达式会被推断为正确的整数类型。例如，`UInt64(0xffff_ffff_ffff_ffff)` 会求值为正确的值，而不是溢出。

并发需要 Swift 5 语言模式，以及提供了相应并发类型的 Swift 标准库版本。在 Apple 平台上，请将部署目标设为至少 iOS 13、macOS 10.15、tvOS 13、watchOS 6 或 visionOS 1。

用 Swift 6.4 编写的目标可以依赖用 Swift 5、Swift 4.2 或 Swift 4 编写的目标，反之亦然。这意味着，如果你有一个划分为多个框架的大型项目，你可以一次迁移一个框架，逐步把代码迁移到更新的语言版本。
