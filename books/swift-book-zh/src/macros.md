# 宏 {#Macros}

使用宏在编译时生成代码。

宏在编译代码时会转换你的源代码，让你不必手工编写重复的代码。编译期间，Swift 会在照常构建代码之前展开代码中的所有宏。

![A diagram showing an overview of macro expansion.  On the left, a stylized representation of Swift code.  On the right, the same code with several lines added by the macro.](macro-expansion)

展开宏始终是一种添加操作：宏会添加新代码，但绝不会删除或修改已有代码。

宏的输入和宏展开的输出都会被检查，以确保它们是语法有效的 Swift 代码。同样，你传给宏的值以及宏生成的代码中的值也会被检查，以确保它们具有正确的类型。此外，如果宏的实现在展开该宏时遇到错误，编译器会将其视为编译错误。这些保证让你更容易理解使用宏的代码，也更容易发现诸如错误使用宏或宏实现存在缺陷之类的问题。

Swift 有两种宏：

- *独立宏*（freestanding macro）单独出现，不附加到某个声明上。

- *附加宏*（attached macro）会修改它所附加到的声明。

调用附加宏和独立宏的方式略有不同，但两者都遵循相同的宏展开模型，并且都用相同的方式实现。下面几节将更详细地介绍这两种宏。

## 独立宏 {#Freestanding-Macros}

要调用独立宏，在宏名称前写一个井号（`#`），并在名称后的括号中写上传给该宏的实参。例如：

```swift
func myFunction() {
    print("Currently running \(#function)")
    #warning("Something's wrong")
}
```

第一行中，`#function` 调用了 Swift 标准库中的 [`function()`][] 宏。编译这段代码时，Swift 会调用该宏的实现，由它把 `#function` 替换为当前函数的名称。运行这段代码并调用 `myFunction()` 时，它会打印“Currently running myFunction()”。第二行中，`#warning` 调用了 Swift 标准库中的 [`warning(_:)`][] 宏，用于生成自定义的编译时警告。

[`function()`]: https://developer.apple.com/documentation/swift/function() [`warning(_:)`]: https://developer.apple.com/documentation/swift/warning(_:)

独立宏可以像 `#function` 那样产生一个值，也可以像 `#warning` 那样在编译时执行一个操作。

## 附加宏 {#Attached-Macros}

要调用附加宏，在宏名称前写一个 at 符号（`@`），并在名称后的括号中写上传给该宏的实参。

附加宏会修改它所附加到的声明。它们向该声明添加代码，例如定义一个新方法，或添加对某个协议的遵循。

例如，考虑下面这段不使用宏的代码：

```swift
struct SundaeToppings: OptionSet {
    let rawValue: Int
    static let nuts = SundaeToppings(rawValue: 1 << 0)
    static let cherry = SundaeToppings(rawValue: 1 << 1)
    static let fudge = SundaeToppings(rawValue: 1 << 2)
}
```

在这段代码中，`SundaeToppings` 选项集（option set）中的每个选项都包含一次对构造器的调用，既重复又需要手工完成。添加新选项时很容易出错，比如在行末敲错数字。

下面是改用宏的版本：

```swift
@OptionSet<Int>
struct SundaeToppings {
    private enum Options: Int {
        case nuts
        case cherry
        case fudge
    }
}
```

这个版本的 `SundaeToppings` 调用了一个 `@OptionSet` 宏。该宏读取私有枚举中的成员列表，为每个选项生成对应的常量列表，并添加对 [`OptionSet`][] 协议的遵循。

[`OptionSet`]: https://developer.apple.com/documentation/swift/optionset

作为对照，下面是 `@OptionSet` 宏展开后的样子。这段代码不是你写的，只有当你特意让 Swift 显示宏展开结果时才会看到它。

```swift
struct SundaeToppings {
    private enum Options: Int {
        case nuts
        case cherry
        case fudge
    }

    typealias RawValue = Int
    var rawValue: RawValue
    init() { self.rawValue = 0 }
    init(rawValue: RawValue) { self.rawValue = rawValue }
    static let nuts: Self = Self(rawValue: 1 << Options.nuts.rawValue)
    static let cherry: Self = Self(rawValue: 1 << Options.cherry.rawValue)
    static let fudge: Self = Self(rawValue: 1 << Options.fudge.rawValue)
}
extension SundaeToppings: OptionSet { }
```

私有枚举之后的所有代码都来自 `@OptionSet` 宏。与前面手工编写的版本相比，使用宏生成所有静态变量的这个 `SundaeToppings` 版本更易读，也更易维护。

## 宏声明 {#Macro-Declarations}

在大多数 Swift 代码中，实现一个符号（例如函数或类型）时并没有单独的声明。然而对于宏，声明和实现是分开的。宏的声明包含它的名称、接收的形参、可以在哪里使用，以及它会生成哪种代码。宏的实现包含通过生成 Swift 代码来展开该宏的代码。

使用 `macro` 关键字引入宏声明。例如，下面是上一个示例中 `@OptionSet` 宏声明的一部分：

```swift
public macro OptionSet<RawType>() =
        #externalMacro(module: "SwiftMacros", type: "OptionSetMacro")
```

第一行指定宏的名称及其实参——名称是 `OptionSet`，它不接收任何实参。第二行使用 Swift 标准库中的 [`externalMacro(module:type:)`][] 宏，告诉 Swift 宏的实现位于何处。在这个例子中，`SwiftMacros` 模块包含一个名为 `OptionSetMacro` 的类型，由它实现 `@OptionSet` 宏。

[`externalMacro(module:type:)`]: https://developer.apple.com/documentation/swift/externalmacro(module:type:)

由于 `OptionSet` 是附加宏，它的名称使用大驼峰命名法，与结构体和类的名称相同。独立宏的名称使用小驼峰命名法，与变量和函数的名称相同。

> 注意：宏始终声明为 `public`。由于声明宏的代码与使用该宏的代码位于不同模块中，非公开的宏没有可以适用的地方。

宏声明定义宏的*角色*（role）——即源代码中可以调用该宏的位置，以及该宏可以生成哪类代码。每个宏都有一个或多个角色，你在宏声明开头的特性中写出这些角色。下面是 `@OptionSet` 声明的更多内容，其中包括指定其角色的特性：

```swift
@attached(member)
@attached(extension, conformances: OptionSet)
public macro OptionSet<RawType>() =
        #externalMacro(module: "SwiftMacros", type: "OptionSetMacro")
```

`@attached` 特性在这个声明中出现两次，每个宏角色对应一次。第一次使用 `@attached(member)` 表示该宏向你应用它的类型添加新成员。`@OptionSet` 宏添加 `OptionSet` 协议所要求的 `init(rawValue:)` 构造器，以及一些额外的成员。第二次使用 `@attached(extension, conformances: OptionSet)` 表明 `@OptionSet` 添加对 `OptionSet` 协议的遵循。`@OptionSet` 宏扩展你应用该宏的类型，以添加对 `OptionSet` 协议的遵循。

对于独立宏，你写出 `@freestanding` 特性来指定其角色：

```swift
@freestanding(expression)
public macro line<T: ExpressibleByIntegerLiteral>() -> T =
        /* ... location of the macro implementation... */
```

上面的 `#line` 宏具有 `expression` 角色。表达式宏产生一个值，或执行编译时操作，例如生成一条警告。

除宏的角色之外，宏声明还提供有关该宏所生成符号的名称的信息。当宏声明提供名称列表时，它保证只生成使用这些名称的声明，这有助于你理解和调试生成的代码。下面是 `@OptionSet` 的完整声明：

```swift
@attached(member, names: named(RawValue), named(rawValue),
        named(`init`), arbitrary)
@attached(extension, conformances: OptionSet)
public macro OptionSet<RawType>() =
        #externalMacro(module: "SwiftMacros", type: "OptionSetMacro")
```

在上面的声明中，`@attached(member)` 宏针对 `@OptionSet` 宏生成的每个符号，在 `names:` 标签之后列出了实参。该宏添加名为 `RawValue`、`rawValue` 和 `init` 的符号声明——由于这些名称是事先已知的，宏声明显式列出了它们。

宏声明还在名称列表之后包含 `arbitrary`，允许该宏生成这样的声明：其名称直到你使用宏时才确定。例如，当 `@OptionSet` 宏应用于上面的 `SundaeToppings` 时，它会生成与枚举成员 `nuts`、`cherry` 和 `fudge` 相对应的类型属性。

有关更多信息，包括宏角色的完整列表，请参阅 [attached](attributes.md#attached) 和 [freestanding](attributes.md#freestanding)，二者都在 [特性](attributes.md) 中。

## 宏展开 {#Macro-Expansion}

构建使用宏的 Swift 代码时，编译器会调用宏的实现来展开它们。

![Diagram showing the four steps of expanding macros.  The input is Swift source code.  This becomes a tree, representing the code's structure.  The macro implementation adds branches to the tree.  The result is Swift source with additional code.](macro-expansion-full)

具体来说，Swift 按以下方式展开宏：

1. 编译器读取代码，创建该语法的内存表示。

1. 编译器把内存表示的一部分发送给宏实现，由它展开该宏。

1. 编译器用宏调用展开后的形式替换该宏调用。

1. 编译器使用展开后的源代码继续编译。

下面具体走一遍这些步骤，考虑以下代码：

```swift
let magicNumber = #fourCharacterCode("ABCD")
```

`#fourCharacterCode` 宏接收一个长度为四个字符的字符串，并返回一个无符号 32 位整数，该整数由字符串中的各个 ASCII 值拼接而成。某些文件格式使用这样的整数来标识数据，因为它们紧凑，而且在调试器中仍然可读。下面的 [实现宏](macros.md#Implementing-a-Macro) 一节展示了如何实现这个宏。

为了展开上面代码中的宏，编译器读取该 Swift 文件，并创建这份代码的内存表示，称为*抽象语法树*（abstract syntax tree），简称 AST。AST 让代码的结构显式呈现出来，从而使编写与该结构交互的代码（例如编译器或宏实现）更容易。下面是上面代码的 AST 表示，为简洁起见省略了一些额外细节：

![A tree diagram, with a constant as the root element.  The constant has a name, magic number, and a value.  The constant's value is a macro call.  The macro call has a name, fourCharacterCode, and arguments.  The argument is a string literal, ABCD.](macro-ast-original)

上图展示了这段代码的结构如何在内存中表示。AST 中的每个元素都对应源代码的一部分。“Constant declaration”AST 元素下面有两个子元素，代表常量声明的两个部分：它的名称和它的值。“Macro call”元素有子元素，分别代表宏的名称和传给该宏的实参列表。

在构造这个 AST 的过程中，编译器会检查源代码是否是有效的 Swift 代码。例如，`#fourCharacterCode` 接收单个实参，该实参必须是字符串。如果你试图传入整数实参，或者忘记在字符串字面量末尾写引号（`"`），就会在流程中的这一步得到错误。

编译器找到代码中调用宏的位置，并加载实现这些宏的外部二进制文件。对于每次宏调用，编译器把 AST 的一部分传给该宏的实现。下面是这个部分 AST 的表示：

![A tree diagram, with a macro call as the root element.  The macro call has a name, fourCharacterCode, and arguments.  The argument is a string literal, ABCD.](macro-ast-input)

`#fourCharacterCode` 宏的实现在展开宏时读取这个部分 AST 作为输入。宏的实现只对作为输入接收的部分 AST 进行操作，这意味着无论宏前后是什么代码，宏的展开方式始终相同。这一限制有助于让宏展开更容易理解，也有助于让代码构建得更快，因为 Swift 可以避免展开没有变化的宏。

Swift 通过限制实现宏的代码，帮助宏作者避免意外读取其他输入：

- 传给宏实现的 AST 只包含表示该宏的 AST 元素，不包含宏前后任何代码。

- 宏实现运行在沙盒环境中，该环境阻止它访问文件系统或网络。

除了这些保护措施之外，宏的作者还有责任不读取或修改宏输入之外的任何内容。例如，宏的展开绝不能依赖当前时间。

`#fourCharacterCode` 的实现生成一个包含展开后代码的新 AST。下面是这段代码返回给编译器的内容：

![A tree diagram with the integer literal 1145258561 of type UInt32.](macro-ast-output)

编译器收到这个展开结果后，会用包含宏展开结果的元素替换包含宏调用的 AST 元素。宏展开之后，编译器再次检查，确保程序仍然是语法有效的 Swift 代码，并且所有类型都正确。这样得到的最终 AST 可以照常编译：

![A tree diagram, with a constant as the root element.  The constant has a name, magic number, and a value.  The constant's value is the integer literal 1145258561 of type UInt32.](macro-ast-result)

这个 AST 对应于如下 Swift 代码：

```swift
let magicNumber = 1145258561 as UInt32
```

在这个示例中，输入的源代码只有一个宏，但真实的程序可能有多处使用同一个宏，也有多处调用不同的宏。编译器一次展开一个宏。

如果一个宏出现在另一个宏内部，外层宏会先展开——这样外层宏就能在内层宏展开之前修改它。

## 实现宏 {#Implementing-a-Macro}

要实现一个宏，你需要创建两个组件：一个执行宏展开的类型，以及一个声明该宏、将其作为 API 公开的库。这些部分与使用该宏的代码分开构建，即使宏及其客户端是一起开发的也是如此，因为宏实现是作为构建宏客户端的一部分运行的。

要使用 Swift Package Manager 创建新宏，请运行 `swift package init --type macro`——这会创建若干文件，包括宏实现和宏声明的模板。

要向现有项目添加宏，请按如下方式编辑 `Package.swift` 文件的开头：

- 在 `swift-tools-version` 注释中把 Swift 工具版本设置为 5.9 或更高版本。
- 导入 `CompilerPluginSupport` 模块。
- 在 `platforms` 列表中把 macOS 10.15 列为最低部署目标。

下面的代码展示了一个示例 `Package.swift` 文件的开头。

```swift
// swift-tools-version: 5.9

import PackageDescription
import CompilerPluginSupport

let package = Package(
    name: "MyPackage",
    platforms: [ .iOS(.v17), .macOS(.v13)],
    // ...
)
```

接下来，在现有的 `Package.swift` 文件中添加一个用于宏实现的目标，以及一个用于宏库的目标。例如，你可以添加类似下面的内容，并把名称改成与你的项目匹配：

```swift
targets: [
    // Macro implementation that performs the source transformations.
    .macro(
        name: "MyProjectMacros",
        dependencies: [
            .product(name: "SwiftSyntaxMacros", package: "swift-syntax"),
            .product(name: "SwiftCompilerPlugin", package: "swift-syntax")
        ]
    ),

    // Library that exposes a macro as part of its API.
    .target(name: "MyProject", dependencies: ["MyProjectMacros"]),
]
```

上面的代码定义了两个目标：`MyProjectMacros` 包含宏的实现，`MyProject` 使这些宏可用。

宏的实现使用 [SwiftSyntax][] 模块，通过 AST 以结构化方式与 Swift 代码交互。如果你使用 Swift Package Manager 创建了新的宏包，生成的 `Package.swift` 文件会自动包含对 SwiftSyntax 的依赖。如果你是在向现有项目添加宏，请在 `Package.swift` 文件中添加对 SwiftSyntax 的依赖：

[SwiftSyntax]: https://github.com/swiftlang/swift-syntax

```swift
dependencies: [
    .package(url: "https://github.com/swiftlang/swift-syntax", from: "509.0.0")
],
```

根据宏的角色，宏实现要遵循 SwiftSyntax 中相应的协议。例如，考虑上一节中的 `#fourCharacterCode`。下面是一个实现该宏的结构体：

```swift
import SwiftSyntax
import SwiftSyntaxMacros

public struct FourCharacterCode: ExpressionMacro {
    public static func expansion(
        of node: some FreestandingMacroExpansionSyntax,
        in context: some MacroExpansionContext
    ) throws -> ExprSyntax {
        guard let argument = node.argumentList.first?.expression,
              let segments = argument.as(StringLiteralExprSyntax.self)?.segments,
              segments.count == 1,
              case .stringSegment(let literalSegment)? = segments.first
        else {
            throw CustomError.message("Need a static string")
        }

        let string = literalSegment.content.text
        guard let result = fourCharacterCode(for: string) else {
            throw CustomError.message("Invalid four-character code")
        }

        return "\(raw: result) as UInt32"
    }
}

private func fourCharacterCode(for characters: String) -> UInt32? {
    guard characters.count == 4 else { return nil }

    var result: UInt32 = 0
    for character in characters {
        result = result << 8
        guard let asciiValue = character.asciiValue else { return nil }
        result += UInt32(asciiValue)
    }
    return result
}
enum CustomError: Error { case message(String) }
```

如果你要把这个宏添加到现有的 Swift Package Manager 项目中，请添加一个类型，作为该宏目标的入口点，并列出该目标所定义的宏：

```swift
import SwiftCompilerPlugin

@main
struct MyProjectMacros: CompilerPlugin {
    var providingMacros: [Macro.Type] = [FourCharacterCode.self]
}
```

`#fourCharacterCode` 宏是一个产生表达式的独立宏，因此实现它的 `FourCharacterCode` 类型遵循 `ExpressionMacro` 协议。`ExpressionMacro` 协议有一个要求，即用于展开 AST 的 `expansion(of:in:)` 方法。有关宏角色及其对应的 SwiftSyntax 协议的列表，请参阅 [attached](attributes.md#attached) 和 [freestanding](attributes.md#freestanding)，二者都在 [特性](attributes.md) 中。

为了展开 `#fourCharacterCode` 宏，Swift 会把使用该宏的代码的 AST 发送给包含宏实现的库。在该库内部，Swift 调用 `FourCharacterCode.expansion(of:in:)`，并把 AST 和上下文作为实参传给该方法。`expansion(of:in:)` 的实现找到传给 `#fourCharacterCode` 的字符串实参，并计算出对应的 32 位无符号整数字面量值。

在上面的示例中，第一个 `guard` 块从 AST 中提取字符串字面量，并把该 AST 元素赋给 `literalSegment`。第二个 `guard` 块调用私有的 `fourCharacterCode(for:)` 函数。如果宏使用不当，这两个块都会抛出错误——错误消息会在格式错误的调用点处变成编译器错误。例如，如果你尝试以 `#fourCharacterCode("AB" + "CD")` 调用该宏，编译器会显示错误“Need a static string”。

`expansion(of:in:)` 方法返回一个 `ExprSyntax` 实例，这是 SwiftSyntax 中表示 AST 里表达式的类型。由于该类型遵循 `StringLiteralConvertible` 协议，宏实现使用字符串字面量作为一种轻量的语法来创建其结果。你从宏实现返回的所有 SwiftSyntax 类型都遵循 `StringLiteralConvertible`，因此在实现任何类型的宏时都可以采用这种方式。

## 开发与调试宏 {#Developing-and-Debugging-Macros}

宏非常适合采用测试来开发：它们把一个 AST 转换为另一个 AST，不依赖任何外部状态，也不更改任何外部状态。此外，你可以从字符串字面量创建语法节点，这简化了为测试准备输入的工作。你还可以读取 AST 的 `description` 属性，得到一个可与期望值比较的字符串。例如，下面是前几节中 `#fourCharacterCode` 宏的一个测试：

```swift
let source: SourceFileSyntax =
    """
    let abcd = #fourCharacterCode("ABCD")
    """

let file = BasicMacroExpansionContext.KnownSourceFile(
    moduleName: "MyModule",
    fullFilePath: "test.swift"
)

let context = BasicMacroExpansionContext(sourceFiles: [source: file])

let transformedSF = source.expand(
    macros:["fourCharacterCode": FourCharacterCode.self],
    in: context
)

let expectedDescription =
    """
    let abcd = 1145258561 as UInt32
    """

precondition(transformedSF.description == expectedDescription)
```

上面的示例使用先决条件来测试该宏，但你也可以改用测试框架。
