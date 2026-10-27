# 文档修订历史 {#Document-Revision-History}

回顾本书近期的改动。

**2026-09-14**

- 针对 Swift 6.4 更新。
- 在 [仅类协议](protocols.md#Class-Only-Protocols) 一节中增加了关于带父类要求的协议的信息。
- 在 [标识符](lexical-structure.md#Identifiers) 一节中增加了关于原始标识符（raw identifier）的信息。
- 在 [attached](attributes.md#attached) 一节中增加了关于主体宏（body macro）的信息。

**2026-03-23**

- 针对 Swift 6.3 更新。
- 全书各处做了小幅更正和补充。

**2025-12-12**

- 针对 Swift 6.2.3 更新。
- 新增 [export](attributes.md#export) 一节，介绍如何把定义导出到客户端模块。

**2025-11-03**

- 针对 Swift 6.2.1 更新。
- 新增 [整数泛型形参](generic-parameters-and-arguments.md#Integer-Generic-Parameters) 一节，介绍取值为数字的泛型形参。
- 在 [整数](the-basics.md#Integers) 和 [浮点数](the-basics.md#Floating-Point-Numbers) 两节中增加了关于如何在整数和浮点数之间做选择的信息。

**2025-09-15**

- 针对 Swift 6.2 更新。
- 新增 [内存安全](the-basics.md#Memory-Safety) 一节，介绍 Swift 帮助你避免的问题。
- 新增 [模式](control-flow.md#Patterns) 一节，介绍 `if case` 语法。
- 在 [并发](concurrency.md) 一章中增加了关于主 actor、隔离和全局 actor 的信息。
- 新增 [协议的隐式一致性](protocols.md#Implicit-Conformance-to-a-Protocol) 一节，介绍如何在不显式写出协议一致性的情况下遵循常用协议，以及如何抑制隐式一致性。
- 新增 [隐式约束](generics.md#Implicit-Constraints) 一节，介绍要求遵循常用协议的泛型约束。

**2025-03-31**

- 针对 Swift 6.1 更新。
- 新增 [不透明参数类型](opaque-types.md#Opaque-Parameter-Types) 一节，介绍如何把 `some` 用作泛型的轻量语法。
- 在 [available](attributes.md#available) 一节中增加了关于 `noasync` 实参的信息。

**2024-09-23**

- 针对 Swift 6 更新。
- 新增 [preconcurrency](attributes.md#preconcurrency) 一节，介绍如何迁移到严格并发检查。
- 新增 [指定错误类型](error-handling.md#Specifying-the-Error-Type) 一节，介绍如何抛出特定类型的错误。
- 更新了 [宏展开表达式](expressions.md#Macro-Expansion-Expression) 一节，因为现在任何宏都可以用作形参的默认值。
- 在 [访问控制](access-control.md) 一章中增加了关于包级访问的信息。

**2024-03-05**

- 针对 Swift 5.10 更新。
- 在 [委托](protocols.md#Delegation) 一节中增加了关于嵌套协议的信息。
- 在 [UIApplicationMain](attributes.md#UIApplicationMain) 和 [NSApplicationMain](attributes.md#NSApplicationMain) 两节中增加了弃用信息。

**2023-12-11**

- 针对 Swift 5.9.2 更新。
- 在 [形参修饰符](declarations.md#Parameter-Modifiers) 一节中增加了关于 `borrowing` 和 `consuming` 修饰符的信息。
- 在 [声明常量和变量](the-basics.md#Declaring-Constants-and-Variables) 中增加了关于在声明之后给常量赋值的信息。
- 在 [并发](concurrency.md) 一章中增加了更多关于任务、任务组和任务取消的信息。
- 在 [宏](macros.md) 一章中增加了关于在现有的 Swift 包中实现宏的信息。
- 更新了 [attached](attributes.md#attached) 一节，因为扩展宏已经取代了一致性宏。
- 新增 [backDeployed](attributes.md#backDeployed) 一节，介绍向后部署。

**2023-09-18**

- 针对 Swift 5.9 更新。
- 在 [控制流](control-flow.md) 一章和 [条件表达式](expressions.md#Conditional-Expression) 一节中增加了关于 `if` 和 `switch` 表达式的信息。
- 新增 [宏](macros.md) 一章，介绍如何在编译期生成代码。
- 扩充了 [基础](the-basics.md) 中关于可选类型的讨论。
- 在 [Swift 概览](guided-tour.md) 中增加了一个并发示例。
- 在 [不透明类型与装箱协议类型](opaque-types.md) 一章中增加了关于装箱协议类型的信息。
- 在 [结果转换](attributes.md#Result-Transformations) 一节中增加了关于 `buildPartialBlock(first:)` 和 `buildPartialBlock(accumulated:next:)` 方法的信息。
- 在 [available](attributes.md#available) 和 [条件编译块](statements.md#Conditional-Compilation-Block) 的平台列表中加入了 visionOS。
- 调整了形式语法的格式，改用空行分组。

**2023-03-30**

- 针对 Swift 5.8 更新。
- 新增 [延迟执行](control-flow.md#Deferred-Actions) 一节，展示错误处理之外的 `defer`。
- 采用 Swift-DocC 发布。
- 全书各处做了小幅更正和补充。

**2022-09-12**

- 针对 Swift 5.7 更新。
- 新增 [可发送类型](concurrency.md#Sendable-Types) 一节，介绍如何在 actor 和任务之间发送数据，并在 [Sendable](attributes.md#Sendable) 和 [unchecked](attributes.md#unchecked) 两节中增加了关于 `@Sendable` 和 `@unchecked` 特性的信息。
- 新增 [正则表达式字面量](lexical-structure.md#Regular-Expression-Literals) 一节，介绍如何创建正则表达式。
- 在 [可选绑定](the-basics.md#Optional-Binding) 一节中增加了关于 `if`-`let` 简写形式的信息。
- 在 [检查 API 可用性](control-flow.md#Checking-API-Availability) 一节中增加了关于 `#unavailable` 的信息。

**2022-03-14**

- 针对 Swift 5.6 更新。
- 更新了 [显式成员表达式](expressions.md#Explicit-Member-Expression) 一节，增加了关于在链式方法调用和其他后缀表达式周围使用 `#if` 的信息。
- 更新了全书插图的视觉样式。

**2021-09-20**

- 针对 Swift 5.5 更新。
- 在 [并发](concurrency.md) 一章以及 [actor 声明](declarations.md#Actor-Declaration)、[异步函数和方法](declarations.md#Asynchronous-Functions-and-Methods) 和 [await 运算符](expressions.md#Await-Operator) 几节中增加了关于异步函数、任务和 actor 的信息。
- 更新了 [标识符](lexical-structure.md#Identifiers) 一节，增加了关于以下划线开头的标识符的信息。

**2021-04-26**

- 针对 Swift 5.4 更新。
- 新增 [结果构造器](advanced-operators.md#Result-Builders) 和 [resultBuilder](attributes.md#resultBuilder) 两节，介绍结果构造器。
- 新增 [隐式转换为指针类型](expressions.md#Implicit-Conversion-to-a-Pointer-Type) 一节，介绍输入输出形参在函数调用中如何隐式转换为不安全的指针。
- 更新了 [可变参数](functions.md#Variadic-Parameters) 和 [函数声明](declarations.md#Function-Declaration) 两节，因为函数现在可以有多个可变参数。
- 更新了 [隐式成员表达式](expressions.md#Implicit-Member-Expression) 一节，因为隐式成员表达式现在可以串联起来。

**2020-09-16**

- 针对 Swift 5.3 更新。
- 在 [尾随闭包](closures.md#Trailing-Closures) 一节中增加了关于多个尾随闭包的信息，并在 [函数调用表达式](expressions.md#Function-Call-Expression) 一节中增加了关于尾随闭包如何与形参匹配的信息。
- 在 [使用合成实现采纳协议](protocols.md#Adopting-a-Protocol-Using-a-Synthesized-Implementation) 一节中增加了关于枚举的 `Comparable` 合成实现的信息。
- 新增 [上下文 where 子句](generics.md#Contextual-Where-Clauses) 一节，因为现在可以在更多位置编写泛型 `where` 子句。
- 新增 [无主可选引用](automatic-reference-counting.md#Unowned-Optional-References) 一节，介绍如何把无主引用用于可选值。
- 在 [main](attributes.md#main) 一节中增加了关于 `@main` 特性的信息。
- 在 [字面量表达式](expressions.md#Literal-Expression) 一节中增加了 `#filePath`，并更新了关于 `#file` 的讨论。
- 更新了 [逃逸闭包](closures.md#Escaping-Closures) 一节，因为闭包现在可以在更多场景中隐式引用 `self`。
- 更新了 [使用 do-catch 处理错误](error-handling.md#Handling-Errors-Using-Do-Catch) 和 [do 语句](statements.md#Do-Statement) 两节，因为 `catch` 子句现在可以匹配多个错误。
- 增加了更多关于 `Any` 的信息，并将其移入新的 [Any 类型](types.md#Any-Type) 一节。
- 更新了 [属性观察器](properties.md#Property-Observers) 一节，因为延迟属性现在可以有观察器。
- 更新了 [协议声明](declarations.md#Protocol-Declaration) 一节，因为枚举的成员现在可以满足协议要求。
- 更新了 [存储变量观察器和属性观察器](declarations.md#Stored-Variable-Observers-and-Property-Observers) 一节，说明 getter 何时会在观察器之前调用。
- 更新了 [内存安全](memory-safety.md) 一章，提到原子操作。

**2020-03-24**

- 针对 Swift 5.2 更新。
- 在 [key path 表达式](expressions.md#Key-Path-Expression) 一节中增加了关于传递 key path 而不是闭包的信息。
- 新增 [具有特殊名称的方法](declarations.md#Methods-with-Special-Names) 一节，介绍一种语法糖，它让类、结构体和枚举的实例可以用函数调用的语法来使用。
- 更新了 [下标选项](subscripts.md#Subscript-Options) 一节，因为下标现在支持带默认值的形参。
- 更新了 [Self 类型](types.md#Self-Type) 一节，因为 `Self` 现在可以在更多上下文中使用。
- 更新了 [隐式解包可选类型](the-basics.md#Implicitly-Unwrapped-Optionals) 一节，更清楚地说明隐式解包可选类型的值既可以当作可选值使用，也可以当作非可选值使用。

**2019-09-10**

- 针对 Swift 5.1 更新。
- 在 [不透明类型与装箱协议类型](opaque-types.md) 一章中增加了关于以下函数的信息：这些函数指定其返回值所遵循的协议，而不是给出具体的具名返回类型。
- 在 [属性包装器](properties.md#Property-Wrappers) 一节中增加了关于属性包装器的信息。
- 在 [frozen](attributes.md#frozen) 一节中增加了关于为库演进而冻结的枚举和结构体的信息。
- 新增 [隐式返回的函数](functions.md#Functions-With-an-Implicit-Return) 和 [简写 getter 声明](properties.md#Shorthand-Getter-Declaration) 两节，介绍省略 `return` 的函数。
- 在 [类型下标](subscripts.md#Type-Subscripts) 一节中增加了关于在类型上使用下标的信息。
- 更新了 [枚举成员模式](patterns.md#Enumeration-Case-Pattern) 一节，因为枚举成员模式现在可以匹配可选值。
- 更新了 [结构体类型的成员逐一构造器](initialization.md#Memberwise-Initializers-for-Structure-Types) 一节，因为成员逐一构造器现在支持为有默认值的属性省略形参。
- 在 [dynamicMemberLookup](attributes.md#dynamicMemberLookup) 一节中增加了关于在运行时通过 key path 查找的动态成员的信息。
- 在 [条件编译块](statements.md#Conditional-Compilation-Block) 的目标环境列表中加入了 `macCatalyst`。
- 更新了 [Self 类型](types.md#Self-Type) 一节，因为 `Self` 现在可以用于指代当前类、结构体或枚举声明引入的类型。

**2019-03-25**

- 针对 Swift 5.0 更新。
- 新增 [扩展字符串分隔符](strings-and-characters.md#Extended-String-Delimiters) 一节，并更新了 [字符串字面量](lexical-structure.md#String-Literals) 一节，增加了关于扩展字符串分隔符的信息。
- 新增 [dynamicCallable](attributes.md#dynamicCallable) 一节，介绍如何使用 `dynamicCallable` 特性把实例当作函数动态调用。
- 新增 [unknown](attributes.md#unknown) 和 [对将来新增的枚举成员进行切换](statements.md#Switching-Over-Future-Enumeration-Cases) 两节，介绍如何在 switch 语句中使用 `unknown` switch 分支特性处理未来的枚举成员。
- 在 [key path 表达式](expressions.md#Key-Path-Expression) 一节中增加了关于恒等 key path（`\.self`）的信息。
- 在 [条件编译块](statements.md#Conditional-Compilation-Block) 一节中增加了关于在平台条件中使用小于（`<`）运算符的信息。

**2018-09-17**

- 针对 Swift 4.2 更新。
- 在 [遍历枚举的成员](enumerations.md#Iterating-over-Enumeration-Cases) 一节中增加了关于访问枚举所有成员的信息。
- 在 [编译时诊断语句](statements.md#Compile-Time-Diagnostic-Statement) 一节中增加了关于 `#error` 和 `#warning` 的信息。
- 在 [声明特性](attributes.md#Declaration-Attributes) 一节中 `inlinable` 和 `usableFromInline` 特性下面增加了关于内联的信息。
- 在 [声明特性](attributes.md#Declaration-Attributes) 一节中 `dynamicMemberLookup` 特性下面增加了关于在运行时按名称查找的成员的信息。
- 在 [声明特性](attributes.md#Declaration-Attributes) 一节中增加了关于 `requires_stored_property_inits` 和 `warn_unqualified_access` 特性的信息。
- 在 [条件编译块](statements.md#Conditional-Compilation-Block) 一节中增加了关于如何根据所用的 Swift 编译器版本条件编译代码的信息。
- 在 [字面量表达式](expressions.md#Literal-Expression) 一节中增加了关于 `#dsohandle` 的信息。

**2018-03-29**

- 针对 Swift 4.1 更新。
- 在 [等价运算符](advanced-operators.md#Equivalence-Operators) 一节中增加了关于等价运算符合成实现的信息。
- 在 [声明](declarations.md) 一章的 [扩展声明](declarations.md#Extension-Declaration) 一节和 [协议](protocols.md) 一章的 [有条件地遵循协议](protocols.md#Conditionally-Conforming-to-a-Protocol) 一节中增加了关于条件协议一致性的信息。
- 在 [在关联类型的约束中使用协议](generics.md#Using-a-Protocol-in-Its-Associated-Types-Constraints) 一节中增加了关于递归协议约束的信息。
- 在 [条件编译块](statements.md#Conditional-Compilation-Block) 中增加了关于 `canImport()` 和 `targetEnvironment()` 平台条件的信息。

**2017-12-04**

- 针对 Swift 4.0.3 更新。
- 更新了 [key path 表达式](expressions.md#Key-Path-Expression) 一节，因为 key path 现在支持下标分量。

**2017-09-19**

- 针对 Swift 4.0 更新。
- 在 [内存安全](memory-safety.md) 一章中增加了关于内存独占访问的信息。
- 新增 [带泛型 where 子句的关联类型](generics.md#Associated-Types-with-a-Generic-Where-Clause) 一节，因为现在可以用泛型 `where` 子句约束关联类型。
- 在 [字符串和字符](strings-and-characters.md) 一章的 [字符串字面量](strings-and-characters.md#String-Literals) 一节和 [词法结构](lexical-structure.md) 一章的 [字符串字面量](lexical-structure.md#String-Literals) 一节中增加了关于多行字符串字面量的信息。
- 更新了 [声明特性](attributes.md#Declaration-Attributes) 中关于 `objc` 特性的讨论，因为该特性现在在更少的地方会被推断出来。
- 新增 [泛型下标](generics.md#Generic-Subscripts) 一节，因为下标现在可以是泛型的。
- 更新了 [协议](protocols.md) 一章 [协议组合](protocols.md#Protocol-Composition) 一节和 [类型](types.md) 一章 [协议组合类型](types.md#Protocol-Composition-Type) 一节中的讨论，因为协议组合类型现在可以包含父类要求。
- 更新了 [扩展声明](declarations.md#Extension-Declaration) 中关于协议扩展的讨论，因为现在不允许在其中使用 `final`。
- 在 [断言和先决条件](the-basics.md#Assertions-and-Preconditions) 一节中增加了关于先决条件和致命错误的信息。

**2017-03-27**

- 针对 Swift 3.1 更新。
- 新增 [带泛型 where 子句的扩展](generics.md#Extensions-with-a-Generic-Where-Clause) 一节，介绍包含要求的扩展。
- 在 [For-In 循环](control-flow.md#For-In-Loops) 一节中增加了遍历区间的示例。
- 在 [可失败构造器](initialization.md#Failable-Initializers) 一节中增加了可失败数值转换的示例。
- 在 [声明特性](attributes.md#Declaration-Attributes) 一节中增加了关于把 `available` 特性与 Swift 语言版本一起使用的信息。
- 更新了 [函数类型](types.md#Function-Type) 一节中的讨论，指出编写函数类型时不允许使用实参标签。
- 更新了 [条件编译块](statements.md#Conditional-Compilation-Block) 一节中关于 Swift 语言版本号的讨论，因为现在允许使用可选的补丁版本号。
- 更新了 [函数类型](types.md#Function-Type) 一节中的讨论，因为 Swift 现在区分接受多个形参的函数和接受单个元组类型形参的函数。
- 从 [表达式](expressions.md) 一章中删除了“动态类型表达式”一节，因为 `type(of:)` 现在是 Swift 标准库函数。

**2016-10-27**

- 针对 Swift 3.0.1 更新。
- 更新了 [自动引用计数](automatic-reference-counting.md) 一章中关于弱引用和无主引用的讨论。
- 在 [声明修饰符](declarations.md#Declaration-Modifiers) 一节中增加了关于 `unowned`、`unowned(safe)` 和 `unowned(unsafe)` 声明修饰符的信息。
- 在 [Any 和 AnyObject 的类型转换](type-casting.md#Type-Casting-for-Any-and-AnyObject) 一节中增加了一条注释，说明在需要 `Any` 类型的值时如何使用可选值。
- 更新了 [表达式](expressions.md) 一章，把带括号的表达式和元组表达式的讨论分开。

**2016-09-13**

- 针对 Swift 3.0 更新。
- 更新了 [函数](functions.md) 一章和 [函数声明](declarations.md#Function-Declaration) 一节中关于函数的讨论，指出所有形参默认都会获得实参标签。
- 更新了 [高级运算符](advanced-operators.md) 一章中关于运算符的讨论，因为现在要把运算符实现为类型方法而不是全局函数。
- 在 [访问控制](access-control.md) 一章中增加了关于 `open` 和 `fileprivate` 访问级别修饰符的信息。
- 更新了 [函数声明](declarations.md#Function-Declaration) 一节中关于 `inout` 的讨论，指出它出现在形参类型前面，而不是形参名前面。
- 更新了 [逃逸闭包](closures.md#Escaping-Closures) 和 [自动闭包](closures.md#Autoclosures) 两节以及 [特性](attributes.md) 一章中关于 `@noescape` 和 `@autoclosure` 特性的讨论，因为它们现在是类型特性，而不是声明特性。
- 在 [高级运算符](advanced-operators.md) 一章的 [自定义中缀运算符的优先级](advanced-operators.md#Precedence-for-Custom-Infix-Operators) 一节和 [声明](declarations.md) 一章的 [优先级组声明](declarations.md#Precedence-Group-Declaration) 一节中增加了关于运算符优先级组的信息。
- 更新了全书中的讨论，改用 macOS 而不是 OS X，用 `Error` 而不是 `ErrorProtocol`，并用 `ExpressibleByStringLiteral` 之类的协议名而不是 `StringLiteralConvertible`。
- 更新了 [泛型](generics.md) 一章 [泛型 where 子句](generics.md#Generic-Where-Clauses) 一节和 [泛型形参与实参](generic-parameters-and-arguments.md) 一章中的讨论，因为泛型 `where` 子句现在写在声明的末尾。
- 更新了 [逃逸闭包](closures.md#Escaping-Closures) 一节中的讨论，因为闭包现在默认是非逃逸的。
- 更新了 [基础](the-basics.md) 一章 [可选绑定](the-basics.md#Optional-Binding) 一节和 [语句](statements.md) 一章 [while 语句](statements.md#While-Statement) 一节中的讨论，因为 `if`、`while` 和 `guard` 语句现在使用以逗号分隔的条件列表，而不再使用 `where` 子句。
- 在 [控制流](control-flow.md) 一章的 [Switch](control-flow.md#Switch) 一节和 [语句](statements.md) 一章的 [switch 语句](statements.md#Switch-Statement) 一节中增加了关于具有多个模式的 switch 分支的信息。
- 更新了 [函数类型](types.md#Function-Type) 一节中关于函数类型的讨论，因为函数实参标签不再是函数类型的一部分。
- 更新了 [协议](protocols.md) 一章 [协议组合](protocols.md#Protocol-Composition) 一节和 [类型](types.md) 一章 [协议组合类型](types.md#Protocol-Composition-Type) 一节中关于协议组合类型的讨论，改用新的 `Protocol1 & Protocol2` 语法。
- 更新了“动态类型表达式”一节中的讨论，改用新的 `type(of:)` 语法来表示动态类型表达式。
- 更新了关于行控制语句的讨论，在 [行控制语句](statements.md#Line-Control-Statement) 一节中改用 `#sourceLocation(file:line:)` 语法。
- 更新了 [永不返回的函数](declarations.md#Functions-that-Never-Return) 中的讨论，改用新的 `Never` 类型。
- 在 [字面量表达式](expressions.md#Literal-Expression) 一节中增加了关于 playground 字面量的信息。
- 更新了 [输入输出参数](declarations.md#In-Out-Parameters) 一节中的讨论，指出只有非逃逸闭包才能捕获输入输出形参。
- 更新了 [默认形参值](functions.md#Default-Parameter-Values) 一节中关于默认形参的讨论，因为现在不能在函数调用中重新排列它们。
- 在 [特性](attributes.md) 一章中把特性实参改为使用冒号。
- 在 [重新抛出函数与方法](declarations.md#Rethrowing-Functions-and-Methods) 一节中增加了关于在可重新抛出函数的 catch 块内抛出错误的信息。
- 在 [选择器表达式](expressions.md#Selector-Expression) 一节中增加了关于访问 Objective-C 属性 getter 或 setter 的选择器的信息。
- 在 [类型别名声明](declarations.md#Type-Alias-Declaration) 一节中增加了关于泛型类型别名和在协议内使用类型别名的信息。
- 更新了 [函数类型](types.md#Function-Type) 一节中关于函数类型的讨论，指出形参类型两侧的括号是必需的。
- 更新了 [特性](attributes.md) 一章，指出 `@IBAction`、`@IBOutlet` 和 `@NSManaged` 特性隐含 `@objc` 特性。
- 在 [声明特性](attributes.md#Declaration-Attributes) 一节中增加了关于 `@GKInspectable` 特性的信息。
- 更新了 [可选协议要求](protocols.md#Optional-Protocol-Requirements) 一节中关于可选协议要求的讨论，说明它们只用于与 Objective-C 互操作的代码。
- 从 [函数声明](declarations.md#Function-Declaration) 一节中删除了关于在函数形参中显式使用 `let` 的讨论。
- 从 [语句](statements.md) 一章中删除了关于 `Boolean` 协议的讨论，因为该协议已从 Swift 标准库中移除。
- 更正了 [声明特性](attributes.md#Declaration-Attributes) 一节中关于 `@NSApplicationMain` 特性的讨论。

**2016-03-21**

- 针对 Swift 2.2 更新。
- 在 [条件编译块](statements.md#Conditional-Compilation-Block) 一节中增加了关于如何根据所用的 Swift 版本条件编译代码的信息。
- 在 [显式成员表达式](expressions.md#Explicit-Member-Expression) 一节中增加了关于如何区分仅实参名称不同的方法或构造器的信息。
- 在 [选择器表达式](expressions.md#Selector-Expression) 一节中增加了关于 Objective-C 选择器的 `#selector` 语法的信息。
- 更新了关于关联类型的讨论，在 [关联类型](generics.md#Associated-Types) 和 [协议关联类型声明](declarations.md#Protocol-Associated-Type-Declaration) 两节中改用 `associatedtype` 关键字。
- 更新了 [可失败构造器](initialization.md#Failable-Initializers) 一节中关于在实例完全初始化之前返回 `nil` 的构造器的信息。
- 在 [比较运算符](basic-operators.md#Comparison-Operators) 一节中增加了关于比较元组的信息。
- 在 [关键字与标点](lexical-structure.md#Keywords-and-Punctuation) 一节中增加了关于使用关键字作为外部形参名的信息。
- 更新了 [声明特性](attributes.md#Declaration-Attributes) 一节中关于 `@objc` 特性的讨论，指出枚举和枚举成员可以使用该特性。
- 更新了 [运算符](lexical-structure.md#Operators) 一节，加入了关于包含点号的自定义运算符的讨论。
- 在 [重新抛出函数与方法](declarations.md#Rethrowing-Functions-and-Methods) 一节中增加了一条注释，说明可重新抛出函数不能直接抛出错误。
- 在 [属性观察器](properties.md#Property-Observers) 一节中增加了一条注释，说明把属性作为输入输出形参传递时会调用属性观察器。
- 在 [Swift 概览](guided-tour.md) 一章中增加了一节关于错误处理的内容。
- 更新了 [弱引用](automatic-reference-counting.md#Weak-References) 一节中的插图，以更清楚地展示释放过程。
- 删除了关于 C 风格 `for` 循环、`++` 前缀和后缀运算符以及 `--` 前缀和后缀运算符的讨论。
- 删除了关于可变函数实参（variable function arguments）和柯里化函数特殊语法的讨论。

**2015-10-20**

- 针对 Swift 2.1 更新。
- 更新了 [字符串插值](strings-and-characters.md#String-Interpolation) 和 [字符串字面量](lexical-structure.md#String-Literals) 两节，因为字符串插值现在可以包含字符串字面量。
- 新增 [逃逸闭包](closures.md#Escaping-Closures) 一节，介绍 `@noescape` 特性。
- 更新了 [声明特性](attributes.md#Declaration-Attributes) 和 [条件编译块](statements.md#Conditional-Compilation-Block) 两节，加入了关于 tvOS 的信息。
- 在 [输入输出参数](declarations.md#In-Out-Parameters) 一节中增加了关于输入输出形参行为的信息。
- 在 [捕获列表](expressions.md#Capture-Lists) 一节中增加了关于闭包捕获列表中指定的值如何被捕获的信息。
- 更新了 [通过可选链访问属性](optional-chaining.md#Accessing-Properties-Through-Optional-Chaining) 一节，说明通过可选链赋值时的行为。
- 改进了 [自动闭包](closures.md#Autoclosures) 一节中关于自动闭包的讨论。
- 在 [Swift 概览](guided-tour.md) 一章中增加了一个使用 `??` 运算符的示例。

**2015-09-16**

- 针对 Swift 2.0 更新。
- 在 [错误处理](error-handling.md) 一章、[do 语句](statements.md#Do-Statement) 一节、[throw 语句](statements.md#Throw-Statement) 一节、[defer 语句](statements.md#Defer-Statement) 一节和 [try 运算符](expressions.md#Try-Operator) 一节中增加了关于错误处理的信息。
- 更新了 [表示并抛出错误](error-handling.md#Representing-and-Throwing-Errors) 一节，因为现在所有类型都可以遵循 `ErrorType` 协议。
- 在 [把错误转换为可选值](error-handling.md#Converting-Errors-to-Optional-Values) 一节中增加了关于新 `try?` 关键字的信息。
- 在 [枚举](enumerations.md) 一章的 [递归枚举](enumerations.md#Recursive-Enumerations) 一节和 [声明](declarations.md) 一章的 [成员可为任意类型的枚举](declarations.md#Enumerations-with-Cases-of-Any-Type) 一节中增加了关于递归枚举的信息。
- 在 [控制流](control-flow.md) 一章的 [检查 API 可用性](control-flow.md#Checking-API-Availability) 一节和 [语句](statements.md) 一章的 [可用性条件](statements.md#Availability-Condition) 一节中增加了关于 API 可用性检查的信息。
- 在 [控制流](control-flow.md) 一章的 [提前退出](control-flow.md#Early-Exit) 一节和 [语句](statements.md) 一章的 [guard 语句](statements.md#Guard-Statement) 一节中增加了关于新 `guard` 语句的信息。
- 在 [协议](protocols.md) 一章的 [协议扩展](protocols.md#Protocol-Extensions) 一节中增加了关于协议扩展的信息。
- 在 [访问控制](access-control.md) 一章的 [单元测试目标的访问级别](access-control.md#Access-Levels-for-Unit-Test-Targets) 一节中增加了关于单元测试访问控制的信息。
- 在 [模式](patterns.md) 一章的 [可选模式](patterns.md#Optional-Pattern) 一节中增加了关于新的可选模式的信息。
- 更新了 [Repeat-While](control-flow.md#Repeat-While) 一节，加入了关于 `repeat`-`while` 循环的信息。
- 更新了 [字符串和字符](strings-and-characters.md) 一章，因为 `String` 不再遵循 Swift 标准库中的 `CollectionType` 协议。
- 在 [输出常量和变量](the-basics.md#Printing-Constants-and-Variables) 一节中增加了关于新的 Swift 标准库函数 `print(_:separator:terminator)` 的信息。
- 在 [枚举](enumerations.md) 一章的 [隐式赋值的原始值](enumerations.md#Implicitly-Assigned-Raw-Values) 一节和 [声明](declarations.md) 一章的 [成员为原始值类型的枚举](declarations.md#Enumerations-with-Cases-of-a-Raw-Value-Type) 一节中增加了关于原始值为 `String` 的枚举成员行为的信息。
- 在 [自动闭包](closures.md#Autoclosures) 一节中增加了关于 `@autoclosure` 特性——包括其 `@autoclosure(escaping)` 形式——的信息。
- 更新了 [声明特性](attributes.md#Declaration-Attributes) 一节，加入了关于 `@available` 和 `@warn_unused_result` 特性的信息。
- 更新了 [类型特性](attributes.md#Type-Attributes) 一节，加入了关于 `@convention` 特性的信息。
- 在 [可选绑定](the-basics.md#Optional-Binding) 一节中增加了一个使用 `where` 子句的多个可选绑定示例。
- 在 [字符串字面量](lexical-structure.md#String-Literals) 一节中增加了关于为什么使用 `+` 运算符连接字符串字面量会在编译期发生的信息。
- 在 [元类型](types.md#Metatype-Type) 一节中增加了关于比较元类型值以及用它们配合构造器表达式构造实例的信息。
- 在 [用断言调试](the-basics.md#Debugging-with-Assertions) 一节中增加了一条注释，说明用户自定义断言何时被禁用。
- 更新了 [声明特性](attributes.md#Declaration-Attributes) 一节中关于 `@NSManaged` 特性的讨论，因为该特性现在可以应用于某些实例方法。
- 更新了 [可变参数](functions.md#Variadic-Parameters) 一节，因为可变参数现在可以声明在函数形参列表的任意位置。
- 在 [重写可失败构造器](initialization.md#Overriding-a-Failable-Initializer) 一节中增加了关于非可失败构造器如何通过强制解包父类构造器的结果来代理到可失败构造器的信息。
- 在 [成员可为任意类型的枚举](declarations.md#Enumerations-with-Cases-of-Any-Type) 一节中增加了关于把枚举成员当作函数使用的信息。
- 在 [构造器表达式](expressions.md#Initializer-Expression) 一节中增加了关于显式引用构造器的信息。
- 在 [编译器控制语句](statements.md#Compiler-Control-Statements) 一节中增加了关于构建配置和行控制语句的信息。
- 在 [元类型](types.md#Metatype-Type) 一节中增加了一条注释，说明如何用元类型值构造类实例。
- 在 [弱引用](automatic-reference-counting.md#Weak-References) 一节中增加了一条注释，说明弱引用不适合用于缓存。
- 更新了 [类型属性](properties.md#Type-Properties) 一节中的一条注释，提到存储类型属性是延迟初始化的。
- 更新了 [捕获值](closures.md#Capturing-Values) 一节，说明变量和常量在闭包中如何被捕获。
- 更新了 [声明特性](attributes.md#Declaration-Attributes) 一节，说明何时可以对类应用 `@objc` 特性。
- 在 [处理错误](error-handling.md#Handling-Errors) 一节中增加了一条注释，说明执行 `throw` 语句的性能。在 [do 语句](statements.md#Do-Statement) 一节中增加了关于 `do` 语句的类似信息。
- 更新了 [类型属性](properties.md#Type-Properties) 一节，加入了关于类、结构体和枚举的存储类型属性和计算类型属性的信息。
- 更新了 [break 语句](statements.md#Break-Statement) 一节，加入了关于带标签的 break 语句的信息。
- 更新了 [属性观察器](properties.md#Property-Observers) 一节中的一条注释，说明 `willSet` 和 `didSet` 观察器的行为。
- 在 [访问级别](access-control.md#Access-Levels) 一节中增加了一条注释，说明 `private` 访问的范围。
- 在 [弱引用](automatic-reference-counting.md#Weak-References) 一节中增加了一条注释，说明垃圾回收系统与 ARC 在弱引用上的差异。
- 更新了 [字符串字面量中的特殊字符](strings-and-characters.md#Special-Characters-in-String-Literals) 一节，给出了更精确的 Unicode 标量定义。

**2015-04-08**

- 针对 Swift 1.2 更新。
- Swift 现在有原生的 `Set` 集合类型。更多信息请参阅 [集合](collection-types.md#Sets)。
- `@autoclosure` 现在是形参声明的特性，而不是形参类型的特性。另外还新增了 `@noescape` 形参声明特性。更多信息请参阅 [声明特性](attributes.md#Declaration-Attributes)。
- 类型方法和类型属性现在使用 `static` 关键字作为声明修饰符。更多信息请参阅 [类型变量属性](declarations.md#Type-Variable-Properties)。
- Swift 现在包含 `as?` 和 `as!` 可失败向下转型运算符。更多信息请参阅 [检查协议一致性](protocols.md#Checking-for-Protocol-Conformance)。
- 新增了一节指南内容，介绍 [字符串索引](strings-and-characters.md#String-Indices)。
- 从 [溢出运算符](advanced-operators.md#Overflow-Operators) 中删除了溢出除法（`&/`）和溢出取余（`&%`）运算符。
- 更新了常量与常量属性声明和初始化的规则。更多信息请参阅 [常量声明](declarations.md#Constant-Declaration)。
- 更新了字符串字面量中 Unicode 标量的定义。参阅 [字符串字面量中的特殊字符](strings-and-characters.md#Special-Characters-in-String-Literals)。
- 更新了 [区间运算符](basic-operators.md#Range-Operators)，指出起止索引相同的半开区间为空。
- 更新了 [闭包是引用类型](closures.md#Closures-Are-Reference-Types)，澄清变量的捕获规则。
- 更新了 [值溢出](advanced-operators.md#Value-Overflow)，澄清有符号和无符号整数的溢出行为。
- 更新了 [协议声明](declarations.md#Protocol-Declaration)，澄清协议声明的作用域和成员。
- 更新了 [定义捕获列表](automatic-reference-counting.md#Defining-a-Capture-List)，澄清闭包捕获列表中弱引用和无主引用的语法。
- 更新了 [运算符](lexical-structure.md#Operators)，明确提到自定义运算符所支持字符的示例，例如数学运算符、杂项符号和装饰符号（Dingbats）这几个 Unicode 块中的字符。
- 常量现在可以在局部函数作用域中声明而不初始化。它们必须在首次使用前被赋值。更多信息请参阅 [常量声明](declarations.md#Constant-Declaration)。
- 在构造器中，常量属性现在只能赋值一次。更多信息请参阅 [在构造过程中给常量属性赋值](initialization.md#Assigning-Constant-Properties-During-Initialization)。
- 现在可以在单个 `if` 语句中，以逗号分隔的赋值表达式列表的形式写多个可选绑定。更多信息请参阅 [可选绑定](the-basics.md#Optional-Binding)。
- [可选链表达式](expressions.md#Optional-Chaining-Expression) 必须出现在后缀表达式内。
- 协议转换不再局限于 `@objc` 协议。
- 在运行时可能失败的类型转换现在使用 `as?` 或 `as!` 运算符，而保证不会失败的类型转换使用 `as` 运算符。更多信息请参阅 [类型转换运算符](expressions.md#Type-Casting-Operators)。

**2014-10-16**

- 针对 Swift 1.1 更新。
- 新增了一份关于 [可失败构造器](initialization.md#Failable-Initializers) 的完整指南。
- 增加了对协议中 [可失败构造器要求](protocols.md#Failable-Initializer-Requirements) 的说明。
- `Any` 类型的常量和变量现在可以包含函数实例。更新了 [Any 和 AnyObject 的类型转换](type-casting.md#Type-Casting-for-Any-and-AnyObject) 中的示例，展示如何在 `switch` 语句中检查函数类型并转换为该类型。
- 带原始值的枚举现在有 `rawValue` 属性，而不是 `toRaw()` 方法；并且有带 `rawValue` 形参的可失败构造器，而不是 `fromRaw()` 方法。更多信息请参阅 [原始值](enumerations.md#Raw-Values) 和 [成员为原始值类型的枚举](declarations.md#Enumerations-with-Cases-of-a-Raw-Value-Type)。
- 新增了一节关于 [可失败构造器](declarations.md#Failable-Initializers) 的参考内容，这类构造器可以触发初始化失败。
- 自定义运算符现在可以包含 `?` 字符。更新了 [运算符](lexical-structure.md#Operators) 参考，描述修订后的规则。从 [自定义运算符](advanced-operators.md#Custom-Operators) 中删除了关于有效运算符字符集的重复说明。

**2014-08-18**

- 新文档，介绍 Swift 1.0——Apple 用于构建 iOS 和 OS X 应用的新编程语言。
- 新增了一节关于协议中 [构造器要求](protocols.md#Initializer-Requirements) 的内容。
- 新增了一节关于 [仅类协议](protocols.md#Class-Only-Protocols) 的内容。
- [断言和先决条件](the-basics.md#Assertions-and-Preconditions) 现在可以使用字符串插值。删除了与此相反的注释。
- 更新了 [拼接字符串和字符](strings-and-characters.md#Concatenating-Strings-and-Characters) 一节，以反映 `String` 和 `Character` 值不能再与加法运算符（`+`）或加法赋值运算符（`+=`）组合使用。这些运算符现在只用于 `String` 值。请使用 `String` 类型的 `append(_:)` 方法把单个 `Character` 值追加到字符串末尾。
- 在 [声明特性](attributes.md#Declaration-Attributes) 一节中增加了关于 `availability` 特性的信息。
- [可选类型](the-basics.md#Optionals) 现在不再在有值时隐式求值为 `true`、无值时求值为 `false`，以避免处理可选 `Bool` 值时产生混淆。相反，请用 `==` 或 `!=` 运算符显式与 `nil` 比较，以判断可选值是否包含值。
- Swift 现在有 [空合运算符](basic-operators.md#Nil-Coalescing-Operator)（`a ?? b`），它会在可选值存在时解包其值，在可选值为 `nil` 时返回默认值。
- 更新并扩充了 [比较字符串](strings-and-characters.md#Comparing-Strings) 一节，以反映并演示字符串和字符的比较以及前缀/后缀比较现在基于扩展字形簇的 Unicode 规范等价性。
- 现在可以通过 [可选链](optional-chaining.md) 尝试设置属性值、给下标赋值，或调用可变方法或运算符。关于 [通过可选链访问属性](optional-chaining.md#Accessing-Properties-Through-Optional-Chaining) 的信息已相应更新，[通过可选链调用方法](optional-chaining.md#Calling-Methods-Through-Optional-Chaining) 中检查方法调用是否成功的示例也已扩充，以展示如何检查属性设置是否成功。
- 新增了一节关于通过可选链访问 [访问可选类型的下标](optional-chaining.md#Accessing-Subscripts-of-Optional-Type) 的内容。
- 更新了 [访问和修改数组](collection-types.md#Accessing-and-Modifying-an-Array) 一节，指出不能再使用 `+=` 运算符向数组追加单个元素。请改用 `append(_:)` 方法，或用 `+=` 运算符追加只含一个元素的数组。
- 增加了一条注释：[区间运算符](basic-operators.md#Range-Operators) 中 `a...b` 和 `a..<b` 的起始值 `a` 不得大于结束值 `b`。
- 重写了 [继承](inheritance.md) 一章，删除了其中关于构造器重写的入门内容。本章现在更侧重于子类新增功能，以及通过重写修改现有功能。本章中 [重写属性的 getter 和 setter](inheritance.md#Overriding-Property-Getters-and-Setters) 的示例已重写，展示如何重写 `description` 属性。（在子类构造器中修改继承属性默认值的示例已移到 [构造过程](initialization.md) 一章。）
- 更新了 [构造器继承与重写](initialization.md#Initializer-Inheritance-and-Overriding) 一节，指出对指定构造器的重写现在必须用 `override` 修饰符标记。
- 更新了 [必要构造器](initialization.md#Required-Initializers) 一节，指出 `required` 修饰符现在要写在必要构造器的每个子类实现之前，并且必要构造器的要求现在可以由自动继承的构造器满足。
- 中缀 [运算符方法](advanced-operators.md#Operator-Methods) 不再需要 `@infix` 特性。
- 用于 [前缀和后缀运算符](advanced-operators.md#Prefix-and-Postfix-Operators) 的 `@prefix` 和 `@postfix` 特性已被 `prefix` 和 `postfix` 声明修饰符取代。
- 增加了一条注释，说明同一个操作数同时应用前缀和后缀运算符时，[前缀和后缀运算符](advanced-operators.md#Prefix-and-Postfix-Operators) 的应用顺序。
- [复合赋值运算符](advanced-operators.md#Compound-Assignment-Operators) 的运算符函数在定义函数时不再使用 `@assignment` 特性。
- 定义 [自定义运算符](advanced-operators.md#Custom-Operators) 时指定修饰符的顺序已改变。例如，现在写 `prefix operator` 而不是 `operator prefix`。
- 在 [声明修饰符](declarations.md#Declaration-Modifiers) 中增加了关于 `dynamic` 声明修饰符的信息。
- 增加了关于类型推断如何与 [字面量](lexical-structure.md#Literals) 配合工作的信息。
- 增加了更多关于柯里化函数的信息。
- 新增了一章关于 [访问控制](access-control.md) 的内容。
- 更新了 [字符串和字符](strings-and-characters.md) 一章，以反映 Swift 的 `Character` 类型现在表示单个 Unicode 扩展字形簇。新增了一节关于 [扩展字形簇](strings-and-characters.md#Extended-Grapheme-Clusters) 的内容，并增加了关于 [Unicode 标量值](strings-and-characters.md#Unicode-Scalar-Values) 和 [比较字符串](strings-and-characters.md#Comparing-Strings) 的更多信息。
- 更新了 [字符串字面量](strings-and-characters.md#String-Literals) 一节，指出字符串字面量中的 Unicode 标量现在写作 `\u{n}`，其中 `n` 是 0 到 10FFFF 之间的十六进制数，即 Unicode 编码空间的范围。
- `NSString` 的 `length` 属性现在映射到 Swift 原生 `String` 类型上的 `utf16Count`，而不是 `utf16count`。
- Swift 的原生 `String` 类型不再有 `uppercaseString` 或 `lowercaseString` 属性。[字符串和字符](strings-and-characters.md) 中相应的一节已被删除，若干代码示例也已更新。
- 新增了一节关于 [不带实参标签的构造参数](initialization.md#Initializer-Parameters-Without-Argument-Labels) 的内容。
- 新增了一节关于 [必要构造器](initialization.md#Required-Initializers) 的内容。
- 新增了一节关于 [可选元组返回类型](functions.md#Optional-Tuple-Return-Types) 的内容。
- 更新了 [类型标注](the-basics.md#Type-Annotations) 一节，指出可以用一个类型标注在同一行定义多个相关的变量。
- `@optional`、`@lazy`、`@final` 和 `@required` 特性现在是 `optional`、`lazy`、`final` 和 `required` [声明修饰符](declarations.md#Declaration-Modifiers)。
- 更新了整本书，把 `..<` 称为 [半开区间运算符](basic-operators.md#Half-Open-Range-Operator)（而不是“半闭区间运算符”）。
- 更新了 [访问和修改字典](collection-types.md#Accessing-and-Modifying-a-Dictionary) 一节，指出 `Dictionary` 现在有布尔值 `isEmpty` 属性。
- 澄清了定义 [自定义运算符](advanced-operators.md#Custom-Operators) 时可以使用的完整字符列表。
- `nil` 以及布尔值 `true` 和 `false` 现在是 [字面量](lexical-structure.md#Literals)。
- Swift 的 `Array` 类型现在具有完整的值语义。更新了关于 [集合的可变性](collection-types.md#Mutability-of-Collections) 和 [数组](collection-types.md#Arrays) 的信息以反映新方式。还澄清了字符串、数组和字典的赋值与拷贝行为。
- [数组类型的简写语法](collection-types.md#Array-Type-Shorthand-Syntax) 现在写作 `[SomeType]`，而不是 `SomeType[]`。
- 新增了一节关于 [字典类型的简写语法](collection-types.md#Dictionary-Type-Shorthand-Syntax) 的内容，写作 `[KeyType: ValueType]`。
- 新增了一节关于 [集合类型的哈希值](collection-types.md#Hash-Values-for-Set-Types) 的内容。
- [闭包表达式](closures.md#Closure-Expressions) 的示例现在使用全局 `sorted(_:_:)` 函数，而不是全局 `sort(_:_:)` 函数，以反映新的数组值语义。
- 更新了关于 [结构体类型的成员逐一构造器](initialization.md#Memberwise-Initializers-for-Structure-Types) 的信息，澄清即使结构体的存储属性没有默认值，结构体的成员逐一构造器仍然可用。
- 把 [半开区间运算符](basic-operators.md#Half-Open-Range-Operator) 的 `..` 改为 `..<`。
- 增加了 [扩展泛型类型](generics.md#Extending-a-Generic-Type) 的示例。
