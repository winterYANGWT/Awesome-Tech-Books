# 词法结构 {#Lexical-Structure}

使用语法中最低层的组成部分。

Swift 的*词法结构*（lexical structure）描述了什么样的字符序列构成语言的有效词法单元。这些有效的词法单元构成语言最低层的构建块，后续章节用它们来描述语言的其余部分。一个词法单元由标识符、关键字、标点、字面量或运算符组成。

大多数情况下，词法单元是从 Swift 源文件的字符中生成的：在下面指定的语法约束范围内，取输入文本中可能的最长子串。这种行为称为*最长匹配*（longest match）或*最大吞噬*（maximal munch）。

## 空白与注释 {#Whitespace-and-Comments}

空白有两种用途：在源文件中分隔词法单元，以及区分前缀、后缀和中缀运算符（见 [运算符](lexical-structure.md#Operators)），除此之外会被忽略。以下字符被视为空白：空格（U+0020）、换行符（U+000A）、回车符（U+000D）、水平制表符（U+0009）、垂直制表符（U+000B）、换页符（U+000C）以及空字符（U+0000）。

注释被编译器当作空白处理。单行注释以 `//` 开头，一直延续到换行符（U+000A）或回车符（U+000D）为止。多行注释以 `/*` 开头，以 `*/` 结尾。允许多行注释嵌套，但注释标记必须配对。

注释可以包含额外的格式和标记，详见 [Markup Formatting Reference](https://developer.apple.com/library/content/documentation/Xcode/Reference/xcode_markup_formatting_ref/index.html)。

> 空白语法：
>
> *whitespace* → *whitespace-item* *whitespace*_?_ \
> *whitespace-item* → *line-break* \
> *whitespace-item* → *inline-space* \
> *whitespace-item* → *comment* \
> *whitespace-item* → *multiline-comment* \
> *whitespace-item* → U+0000, U+000B, or U+000C
>
> *line-break* → U+000A \
> *line-break* → U+000D \
> *line-break* → U+000D followed by U+000A
>
> *inline-spaces* → *inline-space* *inline-spaces*_?_ \
> *inline-space* → U+0009 or U+0020
>
> *comment* → **`//`** *comment-text* *line-break* \
> *multiline-comment* → **`/*`** *multiline-comment-text* **`*/`**
>
> *comment-text* → *comment-text-item* *comment-text*_?_ \
> *comment-text-item* → Any Unicode scalar value except U+000A or U+000D
>
> *multiline-comment-text* → *multiline-comment-text-item* *multiline-comment-text*_?_ \
> *multiline-comment-text-item* → *multiline-comment* \
> *multiline-comment-text-item* → *comment-text-item* \
> *multiline-comment-text-item* → Any Unicode scalar value except  **`/*`** or  **`*/`**

## 标识符 {#Identifiers}

*标识符*（identifier）以大写或小写字母 A 到 Z、下划线（`_`）、Unicode 基本多文种平面（Basic Multilingual Plane）中的非组合字母数字字符，或基本多文种平面之外且不在专用区（Private Use Area）中的字符开头。首字符之后，还可以使用数字和组合 Unicode 字符。

以单个下划线开头的标识符、第一个实参标签以下划线开头的下标，以及第一个实参标签以下划线开头的构造器，都应视为 internal，即使其声明带有 `public` 访问级别修饰符也是如此。这一约定让框架作者能够把 API 的某些部分标记为客户端不得与之交互或依赖的内容，即使某些限制要求该声明必须为 public。此外，以两个下划线开头的标识符保留给 Swift 编译器和标准库。

要将保留字用作标识符，或在标识符中包含空白之类的特殊字符，请在其前后加上反引号（\`）。例如，`class`、`100` 和 `model & test` 都不是有效的标识符，但 `` `class` ``、`` `100` `` 和 `` `model & test` `` 是有效的。反引号不算标识符的一部分；`` `x` `` 和 `x` 含义相同。

*原始标识符*（raw identifier）可以包含除以下字符之外的任何 Unicode 标量值：

- 反引号（`` ` ``）
- 反斜杠（`\`）
- 空字符（U+0000）
- 水平制表符（U+0009）
- 换行符（U+000A）
- 垂直制表符（U+000B）
- 回车符（U+000D）
- 下一行符（U+0085）
- 不间断空格（U+00A0）
- 欧甘文空格标记（U+1680）
- 半方空铅（U+2000）
- 全方空铅（U+2001）
- 半方空格（U+2002）
- 全方空格（U+2003）
- 三分之一全方空格（U+2004）
- 四分之一全方空格（U+2005）
- 六分之一全方空格（U+2006）
- 数字空格（U+2007）
- 标点空格（U+2008）
- 窄空格（U+2009）
- 极窄空格（U+200A）
- 行分隔符（U+2028）
- 段落分隔符（U+2029）
- 窄不间断空格（U+202F）
- 中等数学空格（U+205F）
- 表意空格（U+3000）

原始标识符可以包含空白，但不能只由空白组成。原始标识符中允许出现从左向右标记（U+200E）或从右向左标记（U+200F），并把它们视为空白。原始标识符可以包含运算符字符，详见 [运算符](lexical-structure.md#Operators)，但不能只由运算符字符组成。

> 注意：在 Swift 6.2 之前，在标识符前后写反引号被称为*转义标识符*（escaped identifier），并且仅限于把关键字用作标识符的情况。

在没有显式形参名的闭包内部，形参会隐式命名为 `$0`、`$1`、`$2` 等。这些名称在闭包的作用域内是有效的标识符。

对于带有属性包装器投射值的属性，编译器会合成以美元符号（`$`）开头的标识符。你的代码可以与这些标识符交互，但不能声明带有该前缀的标识符。关于更多信息，请参阅 [特性](attributes.md) 一章中的 [propertyWrapper](attributes.md#propertyWrapper) 一节。

> 标识符语法：
>
> *identifier* → *identifier-head* *identifier-characters*_?_ \
> *identifier* → *raw-identifier* \
> *identifier* → *implicit-parameter-name* \
> *identifier* → *property-wrapper-projection* \
> *identifier-list* → *identifier* | *identifier* **`,`** *identifier-list*
>
> *identifier-head* → Upper- or lowercase letter A through Z \
> *identifier-head* → **`_`** \
> *identifier-head* → U+00A8, U+00AA, U+00AD, U+00AF, U+00B2–U+00B5, or U+00B7–U+00BA \
> *identifier-head* → U+00BC–U+00BE, U+00C0–U+00D6, U+00D8–U+00F6, or U+00F8–U+00FF \
> *identifier-head* → U+0100–U+02FF, U+0370–U+167F, U+1681–U+180D, or U+180F–U+1DBF \
> *identifier-head* → U+1E00–U+1FFF \
> *identifier-head* → U+200B–U+200D, U+202A–U+202E, U+203F–U+2040, U+2054, or U+2060–U+206F \
> *identifier-head* → U+2070–U+20CF, U+2100–U+218F, U+2460–U+24FF, or U+2776–U+2793 \
> *identifier-head* → U+2C00–U+2DFF or U+2E80–U+2FFF \
> *identifier-head* → U+3004–U+3007, U+3021–U+302F, U+3031–U+303F, or U+3040–U+D7FF \
> *identifier-head* → U+F900–U+FD3D, U+FD40–U+FDCF, U+FDF0–U+FE1F, or U+FE30–U+FE44 \
> *identifier-head* → U+FE47–U+FFFD \
> *identifier-head* → U+10000–U+1FFFD, U+20000–U+2FFFD, U+30000–U+3FFFD, or U+40000–U+4FFFD \
> *identifier-head* → U+50000–U+5FFFD, U+60000–U+6FFFD, U+70000–U+7FFFD, or U+80000–U+8FFFD \
> *identifier-head* → U+90000–U+9FFFD, U+A0000–U+AFFFD, U+B0000–U+BFFFD, or U+C0000–U+CFFFD \
> *identifier-head* → U+D0000–U+DFFFD or U+E0000–U+EFFFD
>
> *identifier-character* → *decimal-digit* \
> *identifier-character* → U+0300–U+036F, U+1DC0–U+1DFF, U+20D0–U+20FF, or U+FE20–U+FE2F \
> *identifier-character* → *identifier-head* \
> *identifier-characters* → *identifier-character* *identifier-characters*_?_
>
> *implicit-parameter-name* → **`$`** *decimal-digits* \
> *property-wrapper-projection* → **`$`** *identifier-characters*
>
> *raw-identifier* → **`` ` ``** *raw-identifier-characters* **`` ` ``** \
> *raw-identifier-character* → Any Unicode scalar value except  **`` ` ``**,  **`\`**, U+0000, U+000A, or U+000D *raw-identifier-characters* → *raw-identifier-character* *raw-identifier-characters*_?_

## 关键字与标点 {#Keywords-and-Punctuation}

以下关键字是保留的，不能用作标识符，除非像上文 [标识符](lexical-structure.md#Identifiers) 中所述那样用反引号转义。除 `inout`、`var` 和 `let` 之外的关键字，在函数声明或函数调用中可以用作形参名，而无需用反引号转义。当某个成员与关键字同名时，对该成员的引用无需用反引号转义，除非在引用该成员与使用该关键字之间存在歧义——例如，`self`、`Type` 和 `Protocol` 在显式成员表达式中具有特殊含义，因此在该上下文中必须用反引号转义。

- 声明中使用的关键字：`associatedtype`、`borrowing`、`class`、`consuming`、`deinit`、`enum`、`extension`、`fileprivate`、`func`、`import`、`init`、`inout`、`internal`、`let`、`nonisolated`、`open`、`operator`、`precedencegroup`、`private`、`protocol`、`public`、`rethrows`、`static`、`struct`、`subscript`、`typealias` 和 `var`。

- 语句中使用的关键字：`break`、`case`、`catch`、`continue`、`default`、`defer`、`do`、`else`、`fallthrough`、`for`、`guard`、`if`、`in`、`repeat`、`return`、`switch`、`throw`、`where` 和 `while`。
- 表达式和类型中使用的关键字：`Any`、`as`、`await`、`catch`、`false`、`is`、`nil`、`rethrows`、`self`、`Self`、`super`、`throw`、`throws`、`true` 和 `try`。
- 模式中使用的关键字：`_`。
- 以数字符号（`#`）开头的关键字：`#available`、`#colorLiteral`、`#else`、`#elseif`、`#endif`、`#fileLiteral`、`#if`、`#imageLiteral`、`#keyPath`、`#selector`、`#sourceLocation`、`#unavailable`。

> 注意：在 Swift 5.9 之前，以下关键字是保留的：`#column`、`#dsohandle`、`#error`、`#fileID`、`#filePath`、`#file`、`#function`、`#line` 和 `#warning`。它们现在在 Swift 标准库中实现为宏：[`column`](https://developer.apple.com/documentation/swift/column())、[`dsohandle`](https://developer.apple.com/documentation/swift/dsohandle())、[`error(_:)`](https://developer.apple.com/documentation/swift/error(_:))、[`fileID`](https://developer.apple.com/documentation/swift/fileID())、[`filePath`](https://developer.apple.com/documentation/swift/filePath())、[`file`](https://developer.apple.com/documentation/swift/file())、[`function`](https://developer.apple.com/documentation/swift/function())、[`line`](https://developer.apple.com/documentation/swift/line()) 和 [`warning(_:)`](https://developer.apple.com/documentation/swift/warning(_:))。

- 特定上下文中保留的关键字：`associativity`、`async`、`convenience`、`didSet`、`dynamic`、`final`、`get`、`indirect`、`infix`、`lazy`、`left`、`mutating`、`none`、`nonmutating`、`optional`、`override`、`package`、`postfix`、`precedence`、`prefix`、`Protocol`、`required`、`right`、`set`、`some`、`Type`、`unowned`、`weak` 和 `willSet`。在语法中出现这些关键字的上下文之外，它们可以用作标识符。

以下词法单元被保留为标点，不能用作自定义运算符：`(`、`)`、`{`、`}`、`[`、`]`、`.`、`,`、`:`、`;`、`=`、`@`、`#`、`&`（作为前缀运算符时）、`->`、`` ` ``、`?` 和 `!`（作为后缀运算符时）。

## 字面量 {#Literals}

*字面量*（literal）是某种类型的值在源代码中的表示，例如数字或字符串。

下面是字面量的一些示例：

```swift
42               // Integer literal
3.14159          // Floating-point literal
"Hello, world!"  // String literal
/Hello, .*/      // Regular expression literal
true             // Boolean literal
```

字面量本身没有类型。相反，字面量会被解析为具有无限精度，而 Swift 的类型推断会尝试为它推断一个类型。例如，在声明 `let x: Int8 = 42` 中，Swift 使用显式类型标注（`: Int8`）推断出整数字面量 `42` 的类型是 `Int8`。如果没有合适的类型信息可用，Swift 会推断该字面量的类型为 Swift 标准库中定义的默认字面量类型之一，即下表所列的类型。为字面量值指定类型标注时，标注的类型必须能够从该字面量值实例化。也就是说，该类型必须遵循下表所列的 Swift 标准库协议。

| 字面量 | 默认类型 | 协议 |
| ------- | ------------ | -------- |
| 整数 | `Int` | `ExpressibleByIntegerLiteral` |
| 浮点数 | `Double` | `ExpressibleByFloatLiteral` |
| 字符串 | `String` | `ExpressibleByStringLiteral`；对于只包含单个 Unicode 标量的字符串字面量，为 `ExpressibleByUnicodeScalarLiteral`；对于只包含单个扩展字形簇的字符串字面量，为 `ExpressibleByExtendedGraphemeClusterLiteral` |
| 正则表达式 | `Regex` | 无 |
| 布尔值 | `Bool` | `ExpressibleByBooleanLiteral` |

例如，在声明 `let str = "Hello, world"` 中，字符串字面量 `"Hello, world"` 的默认推断类型是 `String`。另外，`Int8` 遵循 `ExpressibleByIntegerLiteral` 协议，因此可以用在声明 `let x: Int8 = 42` 中整数字面量 `42` 的类型标注里。

> 字面量语法：
>
> *literal* → *numeric-literal* | *string-literal* | *regular-expression-literal* | *boolean-literal* | *nil-literal*
>
> *numeric-literal* → *signed-integer-literal* | *signed-floating-point-literal* \
> *boolean-literal* → **`true`** | **`false`** \
> *nil-literal* → **`nil`**

### 整数字面量 {#Integer-Literals}

*整数字面量*（integer literal）表示精度未指定的整数值。默认情况下，整数字面量以十进制表示；你可以使用前缀来指定其他进制。二进制字面量以 `0b` 开头，八进制字面量以 `0o` 开头，十六进制字面量以 `0x` 开头。

十进制字面量包含数字 `0` 到 `9`。二进制字面量包含 `0` 和 `1`，八进制字面量包含 `0` 到 `7`，十六进制字面量包含 `0` 到 `9` 以及大写或小写的 `A` 到 `F`。

负整数字面量通过在整数字面量前加减号（`-`）来表示，如 `-42`。

为了可读性，允许在数字之间使用下划线（`_`），但它们会被忽略，因此不影响字面量的值。整数字面量可以以零（`0`）打头，但这些零同样会被忽略，不影响字面量的进制或值。

除非另有说明，整数字面量的默认推断类型是 Swift 标准库类型 `Int`。Swift 标准库还定义了各种大小的有符号和无符号整数类型，详见 [整数](the-basics.md#Integers)。

> 整数字面量语法：
>
> *signed-integer-literal* → **`-`**_?_ *integer-literal* *integer-literal* → *binary-literal* \
> *integer-literal* → *octal-literal* \
> *integer-literal* → *decimal-literal* \
> *integer-literal* → *hexadecimal-literal*
>
> *binary-literal* → **`0b`** *binary-digit* *binary-literal-characters*_?_ \
> *binary-digit* → Digit 0 or 1 \
> *binary-literal-character* → *binary-digit* | **`_`** \
> *binary-literal-characters* → *binary-literal-character* *binary-literal-characters*_?_
>
> *octal-literal* → **`0o`** *octal-digit* *octal-literal-characters*_?_ \
> *octal-digit* → Digit 0 through 7 \
> *octal-literal-character* → *octal-digit* | **`_`** \
> *octal-literal-characters* → *octal-literal-character* *octal-literal-characters*_?_
>
> *decimal-literal* → *decimal-digit* *decimal-literal-characters*_?_ \
> *decimal-digit* → Digit 0 through 9 \
> *decimal-digits* → *decimal-digit* *decimal-digits*_?_ \
> *decimal-literal-character* → *decimal-digit* | **`_`** \
> *decimal-literal-characters* → *decimal-literal-character* *decimal-literal-characters*_?_
>
> *hexadecimal-literal* → **`0x`** *hexadecimal-digit* *hexadecimal-literal-characters*_?_ \
> *hexadecimal-digit* → Digit 0 through 9, a through f, or A through F \
> *hexadecimal-literal-character* → *hexadecimal-digit* | **`_`** \
> *hexadecimal-literal-characters* → *hexadecimal-literal-character* *hexadecimal-literal-characters*_?_

### 浮点数字面量 {#Floating-Point-Literals}

*浮点数字面量*（floating-point literal）表示精度未指定的浮点数值。

默认情况下，浮点数字面量以十进制表示（不带前缀），但也可以以十六进制表示（带 `0x` 前缀）。

十进制浮点数字面量由一系列十进制数字组成，后面跟小数部分、十进制指数，或两者都有。小数部分由小数点（`.`）后跟一系列十进制数字组成。指数由大写或小写的 `e` 前缀后跟一系列十进制数字组成，表示 `e` 前面的值要乘以 10 的多少次方。例如，`1.25e2` 表示 1.25 x 10²，其值为 `125.0`。类似地，`1.25e-2` 表示 1.25 x 10⁻²，其值为 `0.0125`。

十六进制浮点数字面量由 `0x` 前缀、一个可选的十六进制小数部分，以及一个十六进制指数组成。十六进制小数部分由小数点后跟一系列十六进制数字组成。指数由大写或小写的 `p` 前缀后跟一系列十进制数字组成，表示 `p` 前面的值要乘以 2 的多少次方。例如，`0xFp2` 表示 15 x 2²，其值为 `60`。类似地，`0xFp-2` 表示 15 x 2⁻²，其值为 `3.75`。

负浮点数字面量通过在浮点数字面量前加减号（`-`）来表示，如 `-42.5`。

为了可读性，允许在数字之间使用下划线（`_`），但它们会被忽略，因此不影响字面量的值。浮点数字面量可以以零（`0`）打头，但这些零同样会被忽略，不影响字面量的进制或值。

除非另有说明，浮点数字面量的默认推断类型是 Swift 标准库类型 `Double`，它表示 64 位浮点数。Swift 标准库还定义了 `Float` 类型，它表示 32 位浮点数。

> 浮点数字面量语法：
>
> *signed-floating-point-literal* → > **`-`**_?_ *floating-point-literal* *floating-point-literal* → *decimal-literal* *decimal-fraction*_?_ *decimal-exponent*_?_ \
> *floating-point-literal* → *hexadecimal-literal* *hexadecimal-fraction*_?_ *hexadecimal-exponent*
>
> *decimal-fraction* → **`.`** *decimal-literal* \
> *decimal-exponent* → *floating-point-e* *sign*_?_ *decimal-literal*
>
> *hexadecimal-fraction* → **`.`** *hexadecimal-digit* *hexadecimal-literal-characters*_?_ \
> *hexadecimal-exponent* → *floating-point-p* *sign*_?_ *decimal-literal*
>
> *floating-point-e* → **`e`** | **`E`** \
> *floating-point-p* → **`p`** | **`P`** \
> *sign* → **`+`** | **`-`**

### 字符串字面量 {#String-Literals}

字符串字面量是由引号包围的一系列字符。单行字符串字面量由双引号包围，形式如下：

```swift
"<#characters#>"
```

字符串字面量不能包含未转义的双引号（`"`）、未转义的反斜杠（`\`）、回车符或换行符。

多行字符串字面量由三个双引号包围，形式如下：

```swift
"""
<#characters#>
"""
```

与单行字符串字面量不同，多行字符串字面量可以包含未转义的双引号（`"`）、回车符和换行符。但它不能包含三个相邻的未转义双引号。

多行字符串字面量开头的 `"""` 之后的换行不属于该字符串。结束该字面量的 `"""` 之前的换行也不属于该字符串。要创建以换行开头或结尾的多行字符串字面量，请在其第一行或最后一行写一个空行。

多行字符串字面量可以使用空格和制表符的任意组合进行缩进；这些缩进不包含在字符串中。结束该字面量的 `"""` 决定缩进：字面量中的每个非空行都必须以与结束的 `"""` 之前完全相同的缩进开头；制表符和空格之间不会互相转换。在该缩进之后，你可以添加额外的空格和制表符；这些空格和制表符会出现在字符串中。

多行字符串字面量中的换行会规范化为换行符（line feed）。即使你的源文件中混用了回车符和换行符，字符串中的所有换行也会保持一致。

在多行字符串字面量中，在行末写一个反斜杠（`\`）会从字符串中省略该换行。反斜杠与换行之间的任何空白也会被省略。你可以使用这种语法在源代码中硬折行多行字符串字面量，而不改变结果字符串的值。

单行和多行两种形式的字符串字面量都可以使用以下转义序列来包含特殊字符：

- 空字符（`\0`）
- 反斜杠（`\\`）
- 水平制表符（`\t`）
- 换行符（`\n`）
- 回车符（`\r`）
- 双引号（`\"`）
- 单引号（`\'`）
- Unicode 标量（`\u{`*n*`}`），其中 *n* 是一个一到八位的十六进制数

通过把表达式放在反斜杠（`\`）后的圆括号中，可以将表达式的值插入字符串字面量。被插值的表达式可以包含字符串字面量，但不能包含未转义的反斜杠、回车符或换行符。

例如，下面所有字符串字面量的值都相同：

```swift
"1 2 3"
"1 2 \("3")"
"1 2 \(3)"
"1 2 \(1 + 2)"
let x = 3; "1 2 \(x)"
```

由扩展字符串分隔符界定的字符串是由引号以及一组配对的、一个或多个数字符号（`#`）包围的一系列字符。由扩展字符串分隔符界定的字符串有以下形式：

```swift
#"<#characters#>"#

#"""
<#characters#>
"""#
```

在由扩展字符串分隔符界定的字符串中，特殊字符会作为普通字符出现在结果字符串里，而不是作为特殊字符。你可以使用扩展字符串分隔符创建这样的字符串：其中的字符通常具有特殊作用，例如生成字符串插值、开始转义序列或终止字符串。

下面的示例展示了一个字符串字面量和一个由扩展字符串分隔符界定的字符串，它们创建出等价的字符串值：

```swift
let string = #"\(x) \ " \u{2603}"#
let escaped = "\\(x) \\ \" \\u{2603}"
print(string)
// Prints "\(x) \ " \u{2603}".
print(string == escaped)
// Prints "true".
```

如果使用多个数字符号来构成由扩展字符串分隔符界定的字符串，不要在数字符号之间放置空白：

```swift
print(###"Line 1\###nLine 2"###) // OK
print(# # #"Line 1\# # #nLine 2"# # #) // Error
```

使用扩展字符串分隔符创建的多行字符串字面量，其缩进要求与常规多行字符串字面量相同。

字符串字面量的默认推断类型是 `String`。关于 `String` 类型的更多信息，请参阅 [字符串和字符](strings-and-characters.md) 和 [`String`](https://developer.apple.com/documentation/swift/string)。

通过 `+` 运算符拼接的字符串字面量会在编译期拼接。例如，下面示例中 `textA` 和 `textB` 的值相同——不会在运行时执行拼接。

```swift
let textA = "Hello " + "world"
let textB = "Hello world"
```

> 字符串字面量语法：
>
> *string-literal* → *static-string-literal* | *interpolated-string-literal*
>
> *string-literal-opening-delimiter* → *extended-string-literal-delimiter*_?_ **`"`** \
> *string-literal-closing-delimiter* → **`"`** *extended-string-literal-delimiter*_?_
>
> *static-string-literal* → *string-literal-opening-delimiter* *quoted-text*_?_ *string-literal-closing-delimiter* \
> *static-string-literal* → *multiline-string-literal-opening-delimiter* *multiline-quoted-text*_?_ *multiline-string-literal-closing-delimiter*
>
> *multiline-string-literal-opening-delimiter* → *extended-string-literal-delimiter*_?_ **`"""`** \
> *multiline-string-literal-closing-delimiter* → **`"""`** *extended-string-literal-delimiter*_?_ \
> *extended-string-literal-delimiter* → **`#`** *extended-string-literal-delimiter*_?_
>
> *quoted-text* → *quoted-text-item* *quoted-text*_?_ \
> *quoted-text-item* → *escaped-character* \
> *quoted-text-item* → Any Unicode scalar value except  **`"`**,  **`\`**, U+000A, or U+000D
>
> *multiline-quoted-text* → *multiline-quoted-text-item* *multiline-quoted-text*_?_ \
> *multiline-quoted-text-item* → *escaped-character* \
> *multiline-quoted-text-item* → Any Unicode scalar value except  **`\`** \
> *multiline-quoted-text-item* → *escaped-newline*
>
> *interpolated-string-literal* → *string-literal-opening-delimiter* *interpolated-text*_?_ *string-literal-closing-delimiter* \
> *interpolated-string-literal* → *multiline-string-literal-opening-delimiter* *multiline-interpolated-text*_?_ *multiline-string-literal-closing-delimiter*
>
> *interpolated-text* → *interpolated-text-item* *interpolated-text*_?_ \
> *interpolated-text-item* → **`\(`** *expression* **`)`** | *quoted-text-item*
>
> *multiline-interpolated-text* → *multiline-interpolated-text-item* *multiline-interpolated-text*_?_ \
> *multiline-interpolated-text-item* → **`\(`** *expression* **`)`** | *multiline-quoted-text-item*
>
> *escape-sequence* → **`\`** *extended-string-literal-delimiter* \
> *escaped-character* → *escape-sequence* **`0`** | *escape-sequence* **`\`** | *escape-sequence* **`t`** | *escape-sequence* **`n`** | *escape-sequence* **`r`** | *escape-sequence* **`"`** | *escape-sequence* **`'`** \
> *escaped-character* → *escape-sequence* **`u`** **`{`** *unicode-scalar-digits* **`}`** \
> *unicode-scalar-digits* → Between one and eight hexadecimal digits
>
> *escaped-newline* → *escape-sequence* *inline-spaces*_?_ *line-break*

### 正则表达式字面量 {#Regular-Expression-Literals}

正则表达式字面量是由斜杠（`/`）包围的一系列字符，形式如下：

```swift
/<#regular expression#>/
```

正则表达式字面量不得以未转义的制表符或空格开头，并且不能包含未转义的斜杠（`/`）、回车符或换行符。

在正则表达式字面量中，反斜杠被理解为该正则表达式的一部分，而不只是像字符串字面量中那样的转义字符。它表示其后的特殊字符应按字面解释，或者其后的非特殊字符应按特殊方式解释。例如，`/\(/` 匹配单个左圆括号，`/\d/` 匹配单个数字。

由扩展分隔符界定的正则表达式字面量是由斜杠（`/`）以及一组配对的、一个或多个数字符号（`#`）包围的一系列字符。由扩展分隔符界定的正则表达式字面量有以下形式：

```swift
#/<#regular expression#>/#

#/
<#regular expression#>
/#
```

使用扩展分隔符的正则表达式字面量可以以未转义的空格或制表符开头，可以包含未转义的斜杠（`/`），并且可以跨多行。对于多行正则表达式字面量，起始分隔符必须位于行末，结束分隔符必须单独占一行。在多行正则表达式字面量内部，默认启用扩展正则表达式语法——具体来说，空白会被忽略，并且允许注释。

如果使用多个数字符号来构成由扩展分隔符界定的正则表达式字面量，不要在数字符号之间放置空白：

```swift
let regex1 = ##/abc/##       // OK
let regex2 = # #/abc/# #     // Error
```

如果需要创建一个空的正则表达式字面量，必须使用扩展分隔符语法。

> 正则表达式字面量语法：
>
> *regular-expression-literal* → *regular-expression-literal-opening-delimiter* *regular-expression* *regular-expression-literal-closing-delimiter* \
> *regular-expression* → Any regular expression
>
> *regular-expression-literal-opening-delimiter* → *extended-regular-expression-literal-delimiter*_?_ **`/`** \
> *regular-expression-literal-closing-delimiter* → **`/`** *extended-regular-expression-literal-delimiter*_?_
>
> *extended-regular-expression-literal-delimiter* → **`#`** *extended-regular-expression-literal-delimiter*_?_

## 运算符 {#Operators}

Swift 标准库定义了许多可供你使用的运算符，其中许多在 [基本运算符](basic-operators.md) 和 [高级运算符](advanced-operators.md) 中讨论。本节介绍可以用哪些字符来定义自定义运算符。

自定义运算符可以以 ASCII 字符 `/`、`=`、`-`、`+`、`!`、`*`、`%`、`<`、`>`、`&`、`|`、`^`、`?` 或 `~` 之一开头，也可以以下面语法中定义的 Unicode 字符之一开头（其中包括 *Mathematical Operators*、*Miscellaneous Symbols* 和 *Dingbats* 等 Unicode 区块中的字符）。首字符之后，还可以使用组合 Unicode 字符。

你还可以定义以点（`.`）开头的自定义运算符。这类运算符可以包含更多的点。例如，`.+.` 被视为单个运算符。如果运算符不以点开头，那么它不能在其他位置包含点。例如，`+.+` 被视为 `+` 运算符后跟 `.+` 运算符。

尽管你可以定义包含问号（`?`）的自定义运算符，但它们不能只由单个问号字符组成。此外，尽管运算符可以包含感叹号（`!`），但后缀运算符不能以问号或感叹号开头。

> 注意：词法单元 `=`、`->`、`//`、`/*`、`*/`、`.`，前缀运算符 `<`、`&` 和 `?`，中缀运算符 `?`，以及后缀运算符 `>`、`!` 和 `?` 是保留的。这些词法单元不能被重载，也不能用作自定义运算符。

运算符周围的空白用于确定该运算符是用作前缀运算符、后缀运算符还是中缀运算符。这一行为有以下规则：

- 如果运算符两侧都有空白，或者两侧都没有空白，则它被视为中缀运算符。例如，`a+++b` 和 `a +++ b` 中的 `+++` 运算符被视为中缀运算符。
- 如果运算符只有左侧有空白，则它被视为前缀一元运算符。例如，`a +++b` 中的 `+++` 运算符被视为前缀一元运算符。
- 如果运算符只有右侧有空白，则它被视为后缀一元运算符。例如，`a+++ b` 中的 `+++` 运算符被视为后缀一元运算符。
- 如果运算符左侧没有空白，但紧跟着一个点（`.`），则它被视为后缀一元运算符。例如，`a+++.b` 中的 `+++` 运算符被视为后缀一元运算符（即 `a+++ .b`，而不是 `a +++ .b`）。

就这些规则而言，运算符之前的字符 `(`、`[` 和 `{`，运算符之后的字符 `)`、`]` 和 `}`，以及字符 `,`、`;` 和 `:` 也被视为空白。

如果预定义的 `!` 或 `?` 运算符左侧没有空白，则无论其右侧是否有空白，它都被视为后缀运算符。要把 `?` 用作可选链运算符，它左侧不能有空白。要把 `?` 用在三元条件（`?` `:`）运算符中，它两侧都必须有空白。

如果中缀运算符的某个操作数是正则表达式字面量，那么该运算符两侧都必须有空白。

在某些构造中，以 `<` 或 `>` 开头的运算符可能会被拆分成两个或更多词法单元。剩余部分也按同样方式处理，并且可能再次拆分。因此，在 `Dictionary<String, Array<Int>>` 这样的构造中，你不需要添加空白来消除结尾 `>` 字符之间的歧义。在这个示例中，结尾的 `>` 字符不会被视为单个词法单元，从而不会被误认为位移 `>>` 运算符。

要了解如何定义新的自定义运算符，请参阅 [自定义运算符](advanced-operators.md#Custom-Operators) 和 [运算符声明](declarations.md#Operator-Declaration)。要了解如何重载现有运算符，请参阅 [运算符方法](advanced-operators.md#Operator-Methods)。

> 运算符语法：
>
> *operator* → *operator-head* *operator-characters*_?_ \
> *operator* → *dot-operator-head* *dot-operator-characters*
>
> *operator-head* → **`/`** | **`=`** | **`-`** | **`+`** | **`!`** | **`*`** | **`%`** | **`<`** | **`>`** | **`&`** | **`|`** | **`^`** | **`~`** | **`?`** \
> *operator-head* → U+00A1–U+00A7 \
> *operator-head* → U+00A9 or U+00AB \
> *operator-head* → U+00AC or U+00AE \
> *operator-head* → U+00B0–U+00B1 \
> *operator-head* → U+00B6, U+00BB, U+00BF, U+00D7, or U+00F7 \
> *operator-head* → U+2016–U+2017 \
> *operator-head* → U+2020–U+2027 \
> *operator-head* → U+2030–U+203E \
> *operator-head* → U+2041–U+2053 \
> *operator-head* → U+2055–U+205E \
> *operator-head* → U+2190–U+23FF \
> *operator-head* → U+2500–U+2775 \
> *operator-head* → U+2794–U+2BFF \
> *operator-head* → U+2E00–U+2E7F \
> *operator-head* → U+3001–U+3003 \
> *operator-head* → U+3008–U+3020 \
> *operator-head* → U+3030
>
> *operator-character* → *operator-head* \
> *operator-character* → U+0300–U+036F \
> *operator-character* → U+1DC0–U+1DFF \
> *operator-character* → U+20D0–U+20FF \
> *operator-character* → U+FE00–U+FE0F \
> *operator-character* → U+FE20–U+FE2F \
> *operator-character* → U+E0100–U+E01EF \
> *operator-characters* → *operator-character* *operator-characters*_?_
>
> *dot-operator-head* → **`.`** \
> *dot-operator-character* → **`.`** | *operator-character* \
> *dot-operator-characters* → *dot-operator-character* *dot-operator-characters*_?_
>
> *infix-operator* → *operator* \
> *prefix-operator* → *operator* \
> *postfix-operator* → *operator*
