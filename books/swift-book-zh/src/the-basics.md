# 基础 {#The-Basics}

使用常见的数据类型，编写基本语法。

Swift 提供了许多基本数据类型，包括表示整数的 `Int`、表示浮点值的 `Double`、表示布尔值的 `Bool`，以及表示文本的 `String`。Swift 还提供了三种主要集合类型的强大版本：`Array`、`Set` 和 `Dictionary`，详见 [集合类型](collection-types.md)。

Swift 用变量来按名称存储和引用值。Swift 也大量使用值不能被改变的变量，这类变量称为常量。在处理不需要改变的值时，Swift 全程使用常量，让代码更安全、意图更清晰。

除了常见类型之外，Swift 还引入了元组等高级类型。元组让你可以创建并传递成组的值。你可以用元组把多个值作为单个复合值从函数中返回。

Swift 用可选类型来处理值缺失的情况。可选类型表示“*有*一个值，它是 *x*”或者“根本*没有*值”。可选类型确保代码在使用值之前总是先检查值是否存在，而非可选值则保证永远不会缺失。

Swift 是一门安全的语言，这意味着它让你更容易在开发过程中尽早发现并修复几类 bug，并让你能够保证某些种类的 bug 不会发生。类型安全让你清楚代码所处理的值的类型。如果代码的某一部分需要 `String`，类型安全会阻止你误传一个 `Int`。内存安全确保你只处理有效的数据，而不是未初始化的内存或已析构的对象，并确保你以安全的方式处理这些数据——即使是在同时运行多段代码的程序中也是如此。Swift 的大部分安全检查在构建代码时执行，某些情况下还会在代码运行时执行额外的检查。

## 常量和变量 {#Constants-and-Variables}

常量和变量把一个名字（如 `maximumNumberOfLoginAttempts` 或 `welcomeMessage`）和一个特定类型的值（如数字 `10` 或字符串 `"Hello"`）关联起来。*常量*的值一旦设定就不能改变，而*变量*的值可以在将来设为不同的值。

### 声明常量和变量 {#Declaring-Constants-and-Variables}

常量和变量必须先声明后使用。用 `let` 关键字声明常量，用 `var` 关键字声明变量。下面的例子演示如何用常量和变量记录用户尝试登录的次数：

```swift
let maximumNumberOfLoginAttempts = 10
var currentLoginAttempt = 0
```

这段代码可以读作：

“声明一个名为 `maximumNumberOfLoginAttempts` 的新常量，并给它赋值 `10`。然后声明一个名为 `currentLoginAttempt` 的新变量，并给它初始值 `0`。”

在这个例子中，允许的最大登录尝试次数被声明为常量，因为这个最大值永远不会变。当前登录尝试计数器被声明为变量，因为这个值必须在每次登录尝试失败后递增。

如果代码中存储的值不会改变，就始终用 `let` 关键字把它声明为常量。只有需要存储会变化的值时才使用变量。

声明常量或变量时，可以像上面的例子那样在声明的同时给它一个值。也可以稍后在程序中给它赋初始值，只要保证在第一次读取它之前它已经有值即可。

```swift
var environment = "development"
let maximumNumberOfLoginAttempts: Int
// maximumNumberOfLoginAttempts has no value yet.

if environment == "development" {
    maximumNumberOfLoginAttempts = 100
} else {
    maximumNumberOfLoginAttempts = 10
}
// Now maximumNumberOfLoginAttempts has a value, and can be read.
```

在这个例子中，最大登录尝试次数是常量，其值取决于环境。在开发环境中，它的值是 100；在其他任何环境中，它的值是 10。`if` 语句的两个分支都会用某个值初始化 `maximumNumberOfLoginAttempts`，从而保证这个常量总能得到值。关于这样设置初始值时 Swift 如何检查代码，详见 [常量声明](declarations.md#Constant-Declaration)。

可以在同一行用逗号分隔声明多个常量或多个变量：

```swift
var x = 0.0, y = 0.0, z = 0.0
```

### 类型标注 {#Type-Annotations}

声明常量或变量时，可以提供*类型标注*（type annotation），以明确该常量或变量可以存储哪种值。写类型标注的方法是在常量或变量名后面加一个冒号，然后加一个空格，再加要使用的类型名。

这个例子为一个名为 `welcomeMessage` 的变量提供了类型标注，表明该变量可以存储 `String` 值：

```swift
var welcomeMessage: String
```

声明中的冒号表示“……的类型是……”，所以上面的代码可以读作：

“声明一个名为 `welcomeMessage` 的变量，它的类型是 `String`。”

“类型是 `String`”这句话的意思是“可以存储任何 `String` 值”。可以把它理解为表示“可存储的东西的类型”（或“可存储的东西的种类”）。

现在可以把 `welcomeMessage` 变量设为任何字符串值而不会出错：

```swift
welcomeMessage = "Hello"
```

可以在同一行用逗号分隔定义多个同类型的相关变量，并在最后一个变量名后面写一个类型标注：

```swift
var red, green, blue: Double
```

> 注意：实践中很少需要写类型标注。如果在定义常量或变量时提供了初始值，Swift 几乎总能推断出该常量或变量应使用的类型，详见 [类型安全和类型推断](the-basics.md#Type-Safety-and-Type-Inference)。在上面的 `welcomeMessage` 示例中，没有提供初始值，因此 `welcomeMessage` 变量的类型是用类型标注指定的，而不是从初始值推断出来的。

### 命名常量和变量 {#Naming-Constants-and-Variables}

常量和变量名几乎可以包含任何字符，包括 Unicode 字符：

```swift
let π = 3.14159
let 你好 = "你好世界"
let 🐶🐮 = "dogcow"
```

常量和变量名不能包含空白字符、数学符号、箭头、私有使用区的 Unicode 标量值，也不能包含线条绘制和方框绘制字符。名字也不能以数字开头，但数字可以出现在名字的其他位置。

一旦声明了某个类型的常量或变量，就不能用同一个名字再次声明它，也不能改变它来存储其他类型的值。也不能把常量变成变量，或把变量变成常量。

> 注意：如果需要让常量或变量与 Swift 保留关键字同名，在把该关键字用作名字时用反引号（`` ` ``）把它括起来。不过，除非绝对没有别的选择，否则应避免用关键字作名字。

可以把现有变量的值改为另一个兼容类型的值。在这个例子中，`friendlyWelcome` 的值从 `"Hello!"` 改成了 `"Bonjour!"`：

```swift
var friendlyWelcome = "Hello!"
friendlyWelcome = "Bonjour!"
// friendlyWelcome is now "Bonjour!"
```

与变量不同，常量的值在设定之后不能改变。尝试改变它会在编译代码时报错：

```swift
let languageName = "Swift"
languageName = "Swift++"
// This is a compile-time error: languageName cannot be changed.
```

### 输出常量和变量 {#Printing-Constants-and-Variables}

可以用 `print(_:separator:terminator:)` 函数输出常量或变量的当前值：

```swift
print(friendlyWelcome)
// Prints "Bonjour!"
```

`print(_:separator:terminator:)` 函数是一个全局函数，把一个或多个值输出到合适的位置。例如在 Xcode 中，`print(_:separator:terminator:)` 函数会把输出打印到 Xcode 的“控制台”面板中。`separator` 和 `terminator` 形参有默认值，因此调用这个函数时可以省略它们。默认情况下，该函数会在打印内容后添加一个换行符来结束该行。要在输出值后不换行，可以传空字符串作为 terminator——例如 `print(someValue, terminator: "")`。关于带默认值的形参，详见 [默认形参值](functions.md#Default-Parameter-Values)。

Swift 使用*字符串插值*（string interpolation）把常量或变量的名字作为占位符包含在较长的字符串中，并让 Swift 用该常量或变量的当前值替换它。把名字放在圆括号中，并在左括号前用反斜杠转义：

```swift
print("The current value of friendlyWelcome is \(friendlyWelcome)")
// Prints "The current value of friendlyWelcome is Bonjour!"
```

> 注意：字符串插值可用的所有选项都在 [字符串插值](strings-and-characters.md#String-Interpolation) 中介绍。

## 注释 {#Comments}

用注释在代码中加入不可执行的文字，作为给自己的说明或提醒。编译代码时，Swift 编译器会忽略注释。

Swift 中的注释与 C 语言中的注释非常相似。单行注释以两个正斜杠（`//`）开头：

```swift
// This is a comment.
```

多行注释以正斜杠后跟星号（`/*`）开头，以星号后跟正斜杠（`*/`）结尾：

```swift
/* This is also a comment
but is written over multiple lines. */
```

与 C 语言中的多行注释不同，Swift 中的多行注释可以嵌套在其他多行注释中。写嵌套注释的方法是先开始一个多行注释块，然后在这个块内开始第二个多行注释。随后先结束第二个块，再结束第一个块：

```swift
/* This is the start of the first multiline comment.
    /* This is the second, nested multiline comment. */
This is the end of the first multiline comment. */
```

嵌套多行注释让你可以快速轻松地注释掉大段代码，即使这段代码中已经包含多行注释。

## 分号 {#Semicolons}

与许多其他语言不同，Swift 不要求你在代码中的每条语句后写分号（`;`），不过如果你愿意也可以写。但是，如果想在同一行写多条独立的语句，则*必须*使用分号：

```swift
let cat = "🐱"; print(cat)
// Prints "🐱".
```

## 整数 {#Integers}

*整数*是没有小数部分的数，如 `42` 和 `-23`。整数可以是*有符号的*（正数、零或负数），也可以是*无符号的*（正数或零），其最大值和最小值取决于它的*大小*（用于存储值的位数）。整数类型的名字中包含大小和符号——例如，8 位无符号整数的类型是 `UInt8`，32 位有符号整数的类型是 `Int32`。与 Swift 中的所有类型一样，这些整数类型的名字以大写字母开头。大多数情况下，如果不需要指定确切的整数大小，就使用下面介绍的 `Int` 类型。

整数类型的行为与你手动进行的大多数算术运算一样；整数运算产生的结果不进行近似。这些特性使整数适合计数以及其他表示精确数量的计算——例如，找出文本文件中最长的一行、在游戏中应用得分倍数，或汇总收据上的价格。

虽然整数没有小数部分，但你可以通过计数小数部分来用整数表示带小数的量。例如，可以把 `123` 存储在一个以美分为单位的整数中来表示 $1.23。这种方法称为*定点运算*（fixed-point math），因为小数点在数字中的位置是固定的。在上面的例子中，数字 `123` 被理解为小数点位于最后两位数字之前。

> 注意：在金融或建筑等受监管领域的计算中，或者在要求高精度结果的领域中，你可能需要一种专用数值类型，它按照该领域的要求实现舍入和截断等行为。

### 整数范围 {#Integer-Bounds}

可以用每种整数类型的 `min` 和 `max` 属性访问其最小值和最大值：

```swift
let minValue = UInt8.min  // minValue is equal to 0, and is of type UInt8
let maxValue = UInt8.max  // maxValue is equal to 255, and is of type UInt8
```

这些属性的值属于相应大小的数值类型（如上面例子中的 `UInt8`），因此可以在表达式中与同类型的其他值一起使用。

产生越界结果的计算（例如得到比 `max` 属性更大的数）会停止程序执行，而不是存储无效结果。你也可以显式地让运算溢出，详见 [溢出运算符](advanced-operators.md#Overflow-Operators)。

### Int {#Int}

大多数情况下，不需要为代码选择特定大小的整数。Swift 还提供了另一种整数类型 `Int`，其大小与当前平台的原生字长相同：

- 在 32 位平台上，`Int` 的大小与 `Int32` 相同。
- 在 64 位平台上，`Int` 的大小与 `Int64` 相同。

除非需要处理特定大小的整数，否则在代码中应始终用 `Int` 表示整数值。这有助于代码的一致性和互操作性。即使在 32 位平台上，`Int` 也能存储 `-2,147,483,648` 到 `2,147,483,647` 之间的任何值，足以覆盖许多整数范围。

### UInt {#UInt}

Swift 还提供了一种无符号整数类型 `UInt`，其大小与当前平台的原生字长相同：

- 在 32 位平台上，`UInt` 的大小与 `UInt32` 相同。
- 在 64 位平台上，`UInt` 的大小与 `UInt64` 相同。

> 注意：只有当确实需要一个与平台原生字长大小相同的无符号整数类型时，才使用 `UInt`。如果不是这种情况，首选 `Int`，即使已知要存储的值非负也是如此。一致地使用 `Int` 表示整数值有助于代码的互操作性，避免在不同数值类型之间转换，并且与整数类型推断相符，详见 [类型安全和类型推断](the-basics.md#Type-Safety-and-Type-Inference)。

## 浮点数 {#Floating-Point-Numbers}

*浮点数*有小数部分，如 `3.14159`、`0.1` 和 `-273.15`。Swift 提供了多种浮点数类型，支持不同大小的数值，就像它有不同大小的整数一样。如果不需要指定确切大小，就使用 `Double`。否则，使用名字中包含所需大小的类型，如 `Float16` 或 `Float80`。按照浮点运算的常见术语，`Float` 使用 32 位，`Double` 使用 64 位。这些类型也可以写作 `Float32` 或 `Float64`。例如，图形代码常使用 `Float` 以匹配 GPU 最快的数据类型。有些浮点数类型只在特定平台上受支持，但 `Float` 和 `Double` 在所有平台上都可用。

浮点数让你能处理非常小和非常大的数字，但无法表示该范围内的每一个可能值。与总是产生精确结果的整数运算不同，浮点运算会把结果舍入到最接近的可表示数字。例如，把数字 10,000 存储为 `Float` 时，能表示的下一个更大的数是 10,000.001——这两个数之间的值会舍入为其中之一。数字之间的间隔也是变化的；大数之间的间隔比小数之间的间隔大。例如，0.001 之后的下一个 `Float` 值是 0.0010000002，它与 0.001 的间隔比 10,000 之后的间隔要小。

浮点数有负零、无穷大和负无穷大等值，用于表示计算中的上溢和下溢。它们还有非数（NaN）值，用于表示无效或未定义的结果，如零除以零。这种行为与整数不同：整数在无法表示结果时会停止程序。

如果需要在所有可能值之间保持相同的间隔，或者所做的计算要求精确结果且用不到上面列出的特殊值，那么浮点数可能不是合适的数据类型。可以考虑改用定点数，详见 [整数](the-basics.md#Integers)。

## 类型安全和类型推断 {#Type-Safety-and-Type-Inference}

Swift 程序中的每个值都有类型。存储值的每个位置——包括常量、变量和属性——也都有类型。你可以用类型标注显式写出类型，也可以让 Swift 从初始值推断类型。在代码中提供值的每个位置，该值的类型都必须与使用它的位置匹配。例如，如果代码的某一部分需要 `String`，就不能误传一个 `Int` 给它。这种检查使 Swift 成为一门*类型安全的*语言。

类型安全的语言鼓励你清楚代码所处理的值的类型。一种类型的值永远不会被隐式转换为另一种类型。不过，有些类型可以被显式转换。构建代码时，Swift 会检查代码的类型安全，并把任何类型不匹配标记为错误。

类型检查帮助你在处理不同类型的值时避免错误。但这并不意味着你必须为声明的每个常量和变量指定类型。如果你不指定所需值的类型，Swift 会使用*类型推断*（type inference）来算出合适的类型。类型推断让编译器在编译代码时，仅通过检查你提供的值，就能自动推断出某个表达式的类型。

得益于类型推断，Swift 需要的类型声明比 C 或 Objective-C 等语言少得多。常量和变量仍然有显式类型，但指定类型的大部分工作都由 Swift 替你完成了。

在声明常量或变量时提供初始值，类型推断尤为有用。通常的做法是在声明常量或变量的同时，给它赋一个*字面量值*（或*字面量*）。（字面量值是直接出现在源代码中的值，如下面例子中的 `42` 和 `3.14159`。）

例如，如果把字面量值 `42` 赋给一个新常量而不说明它是什么类型，Swift 会推断你希望这个常量是 `Int`，因为你用一个看起来像整数的数字初始化了它：

```swift
let meaningOfLife = 42
// meaningOfLife is inferred to be of type Int
```

同样，如果没有为浮点字面量指定类型，Swift 会推断你想创建一个 `Double`：

```swift
let pi = 3.14159
// pi is inferred to be of type Double
```

在推断浮点数的类型时，Swift 总是选择 `Double`（而不是 `Float`）。

如果在表达式中混合使用整数字面量和浮点字面量，会从上下文推断出 `Double` 类型：

```swift
let anotherPi = 3 + 0.14159
// anotherPi is also inferred to be of type Double
```

字面量 `3` 本身没有显式类型，因此，由于加法中包含一个浮点字面量，会推断出合适的输出类型 `Double`。

## 数值字面量 {#Numeric-Literals}

整数字面量可以写成：

- 一个*十进制*数，不带前缀
- 一个*二进制*数，带 `0b` 前缀
- 一个*八进制*数，带 `0o` 前缀
- 一个*十六进制*数，带 `0x` 前缀

所有这些整数字面量的十进制值都是 `17`：

```swift
let decimalInteger = 17
let binaryInteger = 0b10001       // 17 in binary notation
let octalInteger = 0o21           // 17 in octal notation
let hexadecimalInteger = 0x11     // 17 in hexadecimal notation
```

浮点字面量可以是十进制的（不带前缀），也可以是十六进制的（带 `0x` 前缀）。小数点两侧必须始终有数字（或十六进制数字）。十进制浮点数还可以有一个可选的*指数*，用大写或小写 `e` 表示；十六进制浮点数必须有指数，用大写或小写 `p` 表示。

对于指数为 `x` 的十进制数，基数乘以 10ˣ：

- `1.25e2` 表示 1.25 x 10²，即 `125.0`。
- `1.25e-2` 表示 1.25 x 10⁻²，即 `0.0125`。

对于指数为 `x` 的十六进制数，基数乘以 2ˣ：

- `0xFp2` 表示 15 x 2²，即 `60.0`。
- `0xFp-2` 表示 15 x 2⁻²，即 `3.75`。

所有这些浮点字面量的十进制值都是 `12.1875`：

```swift
let decimalDouble = 12.1875
let exponentDouble = 1.21875e1
let hexadecimalDouble = 0xC.3p0
```

数值字面量可以包含额外的格式，以便阅读。整数和浮点数都可以用额外的零填充，也可以包含下划线以提高可读性。这两种格式都不影响字面量的实际值：

```swift
let paddedDouble = 000123.456
let oneMillion = 1_000_000
let justOverOneMillion = 1_000_000.000_000_1
```

## 数值类型转换 {#Numeric-Type-Conversion}

在代码中，所有通用的整数常量和变量都应使用 `Int` 类型，即使已知它们非负也是如此。在日常场景中使用默认整数类型，意味着整数常量和变量可以在代码中直接互操作，并且与整数字面量的推断类型一致。

只有在手头的任务确实需要时，才使用其他整数类型——例如因为外部来源的数据有明确大小，或者出于性能、内存占用或其他必要的优化。在这些情况下使用明确大小的类型有助于发现意外的值溢出，并隐含地说明了所使用数据的性质。

### 整数转换 {#Integer-Conversion}

每种数值类型可存入整数常量或变量的数字范围各不相同。`Int8` 常量或变量可以存储 `-128` 到 `127` 之间的数字，而 `UInt8` 常量或变量可以存储 `0` 到 `255` 之间的数字。无法放入某个有大小整数类型的常量或变量的数字，会在编译代码时报错：

```swift
let cannotBeNegative: UInt8 = -1
// UInt8 can't store negative numbers, and so this will report an error
let tooBig: Int8 = Int8.max + 1
// Int8 can't store a number larger than its maximum value,
// and so this will also report an error
```

由于每种数值类型可以存储的值范围不同，你必须在具体情况下显式选择进行数值类型转换。这种显式选择的方式可以防止隐藏的转换错误，并有助于在代码中明确表达类型转换的意图。

要把一种特定的数值类型转换为另一种，可以用现有的值初始化一个所需类型的新数值。在下面的例子中，常量 `twoThousand` 的类型是 `UInt16`，而常量 `one` 的类型是 `UInt8`。它们不能直接相加，因为类型不同。因此，这个例子调用 `UInt16(one)` 创建一个用 `one` 的值初始化的新 `UInt16`，并用这个值代替原来的值：

```swift
let twoThousand: UInt16 = 2_000
let one: UInt8 = 1
let twoThousandAndOne = twoThousand + UInt16(one)
```

因为加法两边现在都是 `UInt16` 类型，所以允许相加。结果常量（`twoThousandAndOne`）被推断为 `UInt16` 类型，因为它是两个 `UInt16` 值之和。

`SomeType(ofInitialValue)` 是调用 Swift 类型的构造器并传入初始值的默认方式。在底层，`UInt16` 有一个接受 `UInt8` 值的构造器，因此用这个构造器从现有的 `UInt8` 创建新的 `UInt16`。不过，这里不能传入*任何*类型——必须是 `UInt16` 为其提供构造器的类型。扩展现有类型以提供接受新类型（包括你自己定义的类型）的构造器，详见 [扩展](extensions.md)。

### 整数和浮点数的转换 {#Integer-and-Floating-Point-Conversion}

整数和浮点数数值类型之间的转换必须显式进行：

```swift
let three = 3
let pointOneFourOneFiveNine = 0.14159
let pi = Double(three) + pointOneFourOneFiveNine
// pi equals 3.14159, and is inferred to be of type Double
```

这里用常量 `three` 的值创建了一个 `Double` 类型的新值，使加法两边类型相同。如果没有这个转换，就不允许相加。

浮点数到整数的转换也必须显式进行。可以用 `Double` 或 `Float` 值初始化整数类型：

```swift
let integerPi = Int(pi)
// integerPi equals 3, and is inferred to be of type Int
```

以这种方式用浮点值初始化新的整数值时，浮点值总是被截断。这意味着 `4.75` 变成 `4`，`-3.9` 变成 `-3`。

> 注意：组合数值常量和变量的规则与数值字面量的规则不同。字面量值 `3` 可以直接与字面量值 `0.14159` 相加，因为数字字面量本身没有显式类型。它们的类型只在编译器求值时才被推断出来。

## 类型别名 {#Type-Aliases}

*类型别名*为现有类型定义另一个名字。用 `typealias` 关键字定义类型别名。

当你希望用上下文更合适的名字来引用现有类型时，类型别名很有用，例如处理来自外部来源的特定大小的数据时：

```swift
typealias AudioSample = UInt16
```

定义类型别名后，可以在任何原本使用原类型名的地方使用这个别名：

```swift
var maxAmplitudeFound = AudioSample.min
// maxAmplitudeFound is now 0
```

这里把 `AudioSample` 定义为 `UInt16` 的别名。由于它是别名，调用 `AudioSample.min` 实际上调用的是 `UInt16.min`，它为 `maxAmplitudeFound` 变量提供了初始值 `0`。

## 布尔值 {#Booleans}

Swift 有一个基本的*布尔*类型，称为 `Bool`。布尔值被称为*逻辑值*，因为它们只能是真或假。Swift 提供两个布尔常量值：`true` 和 `false`：

```swift
let orangesAreOrange = true
let turnipsAreDelicious = false
```

`orangesAreOrange` 和 `turnipsAreDelicious` 的类型被推断为 `Bool`，因为它们是用布尔字面量值初始化的。与上面的 `Int` 和 `Double` 一样，如果在创建常量或变量的同时把它们设为 `true` 或 `false`，就不需要把它们声明为 `Bool`。

在处理 `if` 语句等条件语句时，布尔值特别有用：

```swift
if turnipsAreDelicious {
    print("Mmm, tasty turnips!")
} else {
    print("Eww, turnips are horrible.")
}
// Prints "Eww, turnips are horrible."
```

`if` 语句等条件语句在 [控制流](control-flow.md) 中有更详细的介绍。

Swift 的类型安全阻止用非布尔值替代 `Bool`。下面的例子会报编译时错误：

```swift
let i = 1
if i {
    // this example will not compile, and will report an error
}
```

不过，下面这个替代例子是有效的：

```swift
let i = 1
if i == 1 {
    // this example will compile successfully
}
```

`i == 1` 比较的结果是 `Bool` 类型，因此第二个例子通过了类型检查。`i == 1` 这类比较在 [基本运算符](basic-operators.md) 中讨论。

与 Swift 中其他类型安全的例子一样，这种方式可以避免意外错误，并确保某段代码的意图始终清晰。

## 元组 {#Tuples}

*元组*把多个值组合成一个复合值。元组中的值可以是任何类型，而且不必彼此类型相同。

在这个例子中，`(404, "Not Found")` 是一个描述 *HTTP 状态码*的元组。HTTP 状态码是每次请求网页时 Web 服务器返回的一个特殊值。如果请求的网页不存在，就会返回状态码 `404 Not Found`。

```swift
let http404Error = (404, "Not Found")
// http404Error is of type (Int, String), and equals (404, "Not Found")
```

`(404, "Not Found")` 元组把一个 `Int` 和一个 `String` 组合在一起，为这个 HTTP 状态码提供两个独立的值：一个数字和一个人类可读的描述。它可以被描述为“一个 `(Int, String)` 类型的元组”。

你可以用任意类型组合创建元组，元组中可以包含任意多种不同的类型。没有什么能阻止你使用 `(Int, Int, Int)` 类型的元组、`(String, Bool)` 类型的元组，或者你需要的任何其他组合。

你可以把元组的内容*分解*成单独的常量或变量，然后照常访问它们：

```swift
let (statusCode, statusMessage) = http404Error
print("The status code is \(statusCode)")
// Prints "The status code is 404".
print("The status message is \(statusMessage)")
// Prints "The status message is Not Found".
```

如果只需要元组中的部分值，可以在分解元组时用下划线（`_`）忽略其中的某些部分：

```swift
let (justTheStatusCode, _) = http404Error
print("The status code is \(justTheStatusCode)")
// Prints "The status code is 404".
```

或者，使用从零开始的索引编号访问元组中的各个元素值：

```swift
print("The status code is \(http404Error.0)")
// Prints "The status code is 404".
print("The status message is \(http404Error.1)")
// Prints "The status message is Not Found".
```

可以在定义元组时给各个元素命名：

```swift
let http200Status = (statusCode: 200, description: "OK")
```

如果给元组中的元素命名，就可以用元素名访问这些元素的值：

```swift
print("The status code is \(http200Status.statusCode)")
// Prints "The status code is 200".
print("The status message is \(http200Status.description)")
// Prints "The status message is OK".
```

元组作为函数的返回值特别有用。一个尝试获取网页的函数可能返回 `(Int, String)` 元组类型，以描述页面获取的成功或失败。通过返回包含两个不同值（类型也各不相同）的元组，函数能比只能返回单一类型的单个值提供更多关于其结果的有用信息。更多信息详见 [有多个返回值的函数](functions.md#Functions-with-Multiple-Return-Values)。

> 注意：元组适合用于简单的相关值分组。它们不适合用来创建复杂的数据结构。如果你的数据结构可能比较复杂，应把它建模为类或结构体，而不是元组。更多信息详见 [结构体和类](classes-and-structures.md)。

## 可选类型 {#Optionals}

在值可能缺失的情况下使用*可选类型*（optional）。可选类型表示两种可能：要么*有*一个指定类型的值，你可以解包可选类型来访问该值；要么根本*没有*值。

举一个可能缺失的值的例子：Swift 的 `Int` 类型有一个构造器，尝试把 `String` 值转换为 `Int` 值。然而，只有部分字符串能转换为整数。字符串 `"123"` 可以转换为数值 `123`，但字符串 `"hello, world"` 没有对应的数值。下面的例子使用这个构造器尝试把 `String` 转换为 `Int`：

```swift
let possibleNumber = "123"
let convertedNumber = Int(possibleNumber)
// The type of convertedNumber is "optional Int"
```

因为上面代码中的构造器可能失败，所以它返回一个*可选* `Int`，而不是 `Int`。

要写可选类型，在可选类型所包含的类型名后面写一个问号（`?`）——例如，可选 `Int` 的类型是 `Int?`。可选 `Int` 总是包含某个 `Int` 值，或者根本不包含值。它不能包含其他任何东西，比如 `Bool` 或 `String` 值。

### nil {#nil}

通过给可选变量赋特殊值 `nil`，可以把它设为无值状态：

```swift
var serverResponseCode: Int? = 404
// serverResponseCode contains an actual Int value of 404
serverResponseCode = nil
// serverResponseCode now contains no value
```

如果定义可选变量时没有提供默认值，该变量会自动设为 `nil`：

```swift
var surveyAnswer: String?
// surveyAnswer is automatically set to nil
```

可以用 `if` 语句把可选类型与 `nil` 比较，来判断可选类型是否包含值。使用“等于”运算符（`==`）或“不等于”运算符（`!=`）进行比较。

如果可选类型有值，就认为它“不等于”`nil`：

```swift
let possibleNumber = "123"
let convertedNumber = Int(possibleNumber)

if convertedNumber != nil {
    print("convertedNumber contains some integer value.")
}
// Prints "convertedNumber contains some integer value."
```

不能对非可选的常量或变量使用 `nil`。如果代码中的常量或变量在某些条件下需要处理值缺失，就把它声明为相应类型的可选值。声明为非可选值的常量或变量保证永远不会包含 `nil`。如果试图把 `nil` 赋给非可选值，会得到编译时错误。

这种可选值与非可选值的区分让你能显式标记哪些信息可以缺失，也更容易编写处理缺失值的代码。你不会无意中把可选值当作非可选值，因为这种失误会在编译时产生错误。解包值之后，处理该值的其他代码都不需要再检查 `nil`，因此不必在代码的不同部分重复检查同一个值。

访问可选值时，代码总是同时处理 `nil` 和非 `nil` 的情况。值缺失时有几种处理方式，如下文各节所述：

- 当值为 `nil` 时跳过操作该值的代码。

- 传播 `nil` 值：返回 `nil`，或使用 [可选链](optional-chaining.md) 中介绍的 `?.` 运算符。

- 使用 `??` 运算符提供回退值。

- 使用 `!` 运算符停止程序执行。

> 注意：在 Objective-C 中，`nil` 是指向不存在对象的指针。在 Swift 中，`nil` 不是指针——它表示某种类型的值缺失。*任何*类型的可选类型都可以设为 `nil`，不只是对象类型。

### 可选绑定 {#Optional-Binding}

用可选绑定来判断可选类型是否包含值，如果包含，就把该值作为一个临时常量或变量来使用。可选绑定可以与 `if`、`guard` 和 `while` 语句配合，在单个动作中检查可选类型内部是否有值，并把该值提取到常量或变量中。关于 `if`、`guard` 和 `while` 语句的更多信息，详见 [控制流](control-flow.md)。

为 `if` 语句编写可选绑定的方式如下：

```swift
if let <#constantName#> = <#someOptional#> {
   <#statements#>
}
```

可以把 [可选类型](the-basics.md#Optionals) 一节中的 `possibleNumber` 示例改写为使用可选绑定而不是强制解包：

```swift
if let actualNumber = Int(possibleNumber) {
    print("The string \"\(possibleNumber)\" has an integer value of \(actualNumber)")
} else {
    print("The string \"\(possibleNumber)\" couldn't be converted to an integer")
}
// Prints "The string "123" has an integer value of 123".
```

这段代码可以读作：

“如果 `Int(possibleNumber)` 返回的可选 `Int` 包含值，就把一个名为 `actualNumber` 的新常量设为该可选类型中包含的值。”

如果转换成功，`actualNumber` 常量就可以在 `if` 语句的第一个分支内使用。它已经用可选类型中包含的值初始化，并具有相应的非可选类型。在这个例子中，`Int(possibleNumber)` 的类型是 `Int?`，所以 `actualNumber` 的类型是 `Int`。

如果在访问可选常量或变量包含的值之后不再需要引用原来的可选常量或变量，新常量或变量可以沿用同一个名字：

```swift
let myNumber = Int(possibleNumber)
// Here, myNumber is an optional integer
if let myNumber = myNumber {
    // Here, myNumber is a non-optional integer
    print("My number is \(myNumber)")
}
// Prints "My number is 123".
```

这段代码首先检查 `myNumber` 是否包含值，与上一个例子中的代码一样。如果 `myNumber` 有值，就把一个名为 `myNumber` 的新常量设为该值。在 `if` 语句的主体内部，写 `myNumber` 指的是这个新的非可选常量。在 `if` 语句之前或之后写 `myNumber`，指的则是原来的可选整数常量。

因为这种代码非常常见，可以用更简短的写法解包可选值：只写你要解包的常量或变量名。新的已解包常量或变量会隐式使用与可选值相同的名字。

```swift
if let myNumber {
    print("My number is \(myNumber)")
}
// Prints "My number is 123".
```

可选绑定既可以用于常量，也可以用于变量。如果希望在 `if` 语句的第一个分支内修改 `myNumber` 的值，可以改写为 `if var myNumber`，这样可选类型中包含的值就会以变量而不是常量的形式提供。在 `if` 语句主体内对 `myNumber` 所做的修改只作用于这个局部变量，*不会*影响你解包的那个原始可选常量或变量。

在单个 `if` 语句中，可以根据需要包含任意多个可选绑定和布尔条件，用逗号分隔。如果任一可选绑定的值是 `nil`，或任一布尔条件求值为 `false`，整个 `if` 语句的条件就被视为 `false`。下面两个 `if` 语句是等价的：

```swift
if let firstNumber = Int("4"), let secondNumber = Int("42"), firstNumber < secondNumber && secondNumber < 100 {
    print("\(firstNumber) < \(secondNumber) < 100")
}
// Prints "4 < 42 < 100".

if let firstNumber = Int("4") {
    if let secondNumber = Int("42") {
        if firstNumber < secondNumber && secondNumber < 100 {
            print("\(firstNumber) < \(secondNumber) < 100")
        }
    }
}
// Prints "4 < 42 < 100".
```

在 `if` 语句中用可选绑定创建的常量和变量只在 `if` 语句主体内可用。相比之下，用 `guard` 语句创建的常量和变量在 `guard` 语句之后的代码行中可用，详见 [提前退出](control-flow.md#Early-Exit)。

### 提供回退值 {#Providing-a-Fallback-Value}

处理值缺失的另一种方式是使用空合运算符（`??`）提供默认值。如果 `??` 左边的可选值不是 `nil`，就解包并使用该值；否则使用 `??` 右边的值。例如，下面代码在指定了名字时按名字问候某人，在名字为 `nil` 时使用通用问候语。

```swift
let name: String? = nil
let greeting = "Hello, " + (name ?? "friend") + "!"
print(greeting)
// Prints "Hello, friend!"
```

关于使用 `??` 提供回退值的更多信息，详见 [空合运算符](basic-operators.md#Nil-Coalescing-Operator)。

### 强制解包 {#Force-Unwrapping}

当 `nil` 表示不可恢复的失败（如程序员错误或状态损坏）时，可以在可选值名字末尾加一个感叹号（`!`）来访问可选类型的底层值。这称为*强制解包*（force unwrapping）可选值。对非 `nil` 值强制解包时，结果是其解包后的值。对 `nil` 值强制解包会触发运行时错误。

`!` 实际上是一种更简短的 [`fatalError(_:file:line:)`][] 写法。例如，下面的代码展示了两种等价的做法：

[`fatalError(_:file:line:)`]: https://developer.apple.com/documentation/swift/fatalerror(_:file:line:)

```swift
let possibleNumber = "123"
let convertedNumber = Int(possibleNumber)

let number = convertedNumber!

guard let number = convertedNumber else {
    fatalError("The number was invalid")
}
```

上面两个版本的代码都依赖于 `convertedNumber` 始终包含值。用上述任一做法把这个要求写进代码，就能让代码在运行时检查该要求是否成立。

关于在运行时强制执行数据要求并检查假设的更多信息，详见 [断言和先决条件](the-basics.md#Assertions-and-Preconditions)。

### 隐式解包可选类型 {#Implicitly-Unwrapped-Optionals}

如上所述，可选类型表示常量或变量允许“没有值”。可以用 `if` 语句检查可选类型是否存在值，也可以用可选绑定有条件地解包，从而在值确实存在时访问该可选值。

有时从程序结构可以清楚地看出，可选类型在首次设定值之后将*始终*有值。在这种情况下，去掉每次访问时检查和解包可选值的需要会很有用，因为可以安全地假定它一直都有值。

这类可选类型定义为*隐式解包可选类型*（implicitly unwrapped optional）。写隐式解包可选类型的方法是在要设为可选的类型后面放一个感叹号（`String!`），而不是问号（`String?`）。感叹号不是在用的时候放在可选值名字后面，而是在声明时放在可选类型后面。

当可选值在首次定义后立即确认存在，并且此后可以确定它在每个时刻都存在时，隐式解包可选类型很有用。Swift 中隐式解包可选类型的主要用途是在类构造过程中，详见 [无主引用和隐式解包可选属性](automatic-reference-counting.md#Unowned-References-and-Implicitly-Unwrapped-Optional-Properties)。

如果变量以后有可能变成 `nil`，就不要使用隐式解包可选类型。如果需要在变量的生命周期内检查 `nil` 值，应始终使用普通可选类型。

隐式解包可选类型在底层是一个普通可选类型，但也可以像非可选值一样使用，无需在每次访问时解包可选值。下面的例子展示了可选字符串和隐式解包可选字符串在以显式 `String` 访问其被包装值时的行为差异：

```swift
let possibleString: String? = "An optional string."
let forcedString: String = possibleString! // Requires explicit unwrapping

let assumedString: String! = "An implicitly unwrapped optional string."
let implicitString: String = assumedString // Unwrapped automatically
```

可以把隐式解包可选类型理解为允许在需要时对该可选值进行强制解包。使用隐式解包可选值时，Swift 首先尝试把它当作普通可选值使用；如果它不能作为可选值使用，Swift 就会强制解包该值。在上面的代码中，可选值 `assumedString` 在把其值赋给 `implicitString` 之前被强制解包，因为 `implicitString` 具有显式的非可选类型 `String`。在下面的代码中，`optionalString` 没有显式类型，因此它是普通可选类型。

```swift
let optionalString = assumedString
// The type of optionalString is "String?" and assumedString isn't force-unwrapped.
```

如果隐式解包可选类型是 `nil`，而你试图访问它的被包装值，就会触发运行时错误。结果与对一个不包含值的普通可选类型写感叹号进行强制解包完全相同。

可以用检查普通可选类型的同样方式检查隐式解包可选类型是否为 `nil`：

```swift
if assumedString != nil {
    print(assumedString!)
}
// Prints "An implicitly unwrapped optional string."
```

也可以对隐式解包可选类型使用可选绑定，在单条语句中检查并解包其值：

```swift
if let definiteString = assumedString {
    print(definiteString)
}
// Prints "An implicitly unwrapped optional string."
```

## 内存安全 {#Memory-Safety}

除了上文 [类型安全和类型推断](the-basics.md#Type-Safety-and-Type-Inference) 中介绍的防止类型不匹配的检查之外，Swift 还保护代码不去处理无效内存。这种保护称为*内存安全*，包括以下要求：

- 值在被读取之前已经设定。防止与未初始化的内存区域交互的保护也称为*确定初始化*（definite initialization）。
- 数组和缓冲区只在有效索引处访问。防止越界访问的保护也称为*边界安全*（bounds safety）。
- 只在值的生命周期内访问内存。防止释放后使用错误的保护也称为*生命周期安全*（lifetime safety）。
- 对内存的访问只以可证明安全的方式重叠。防止并发代码中可能的数据竞争的保护也称为*线程安全*（thread safety）。

如果你用过不提供这些保证的语言，可能对上面列表中提到的某些错误和 bug 并不陌生。如果你没遇到过这些问题，那也没关系；Swift 中的安全代码会避免这些问题。关于 Swift 如何确保你设定初始值，详见 [构造过程](initialization.md)；关于 Swift 如何检查并发代码中的内存安全，详见 [并发](concurrency.md)；关于 Swift 如何检查重叠的内存访问，详见 [内存安全](memory-safety.md)。

有时你需要突破安全的边界——例如，由于语言或标准库的限制——因此 Swift 也提供了一些 API 的不安全版本。当你使用名字中包含“unsafe”、“unchecked”或“unmanaged”等词语的类型或方法时，你就要自行承担安全责任。

Swift 中的安全代码仍可能遇到错误和意外失败，这可能会停止程序执行。安全并不能保证代码运行到结束。Swift 提供了几种表示错误和从错误中恢复的方式，将在下文 [错误处理](the-basics.md#Error-Handling) 和 [断言和先决条件](the-basics.md#Assertions-and-Preconditions) 中讨论。不过，在某些情况下，处理错误的*唯一*安全方式就是停止执行。如果需要保证某个服务永远不会意外停止，就应把容错机制纳入其整体架构，使其能够从任何组件意外停止中恢复。

## 错误处理 {#Error-Handling}

用*错误处理*来响应程序在执行过程中可能遇到的错误状况。

与可以用值的存在与否来传达函数成功或失败的可选类型不同，错误处理让你能确定失败的底层原因，并在必要时把错误传播到程序的其他部分。

当函数遇到错误状况时，它会*抛出*错误。该函数的调用者随后可以*捕获*错误并做出适当响应。

```swift
func canThrowAnError() throws {
    // this function may or may not throw an error
}
```

函数在声明中包含 `throws` 关键字，表示它可以抛出错误。调用可能抛出错误的函数时，要在表达式前面加上 `try` 关键字。

Swift 会自动把错误传播出当前作用域，直到被某个 `catch` 子句处理。

```swift
do {
    try canThrowAnError()
    // no error was thrown
} catch {
    // an error was thrown
}
```

`do` 语句会创建一个新的包含作用域，使得错误可以传播到一个或多个 `catch` 子句。

下面是一个用错误处理响应不同错误状况的例子：

```swift
func makeASandwich() throws {
    // ...
}

do {
    try makeASandwich()
    eatASandwich()
} catch SandwichError.outOfCleanDishes {
    washDishes()
} catch SandwichError.missingIngredients(let ingredients) {
    buyGroceries(ingredients)
}
```

在这个例子中，如果没有干净的盘子或缺少任何原料，`makeASandwich()` 函数就会抛出错误。因为 `makeASandwich()` 可能抛出错误，所以函数调用被包在 `try` 表达式中。把函数调用包在 `do` 语句中后，抛出的任何错误都会传播到提供的 `catch` 子句。

如果没有抛出错误，就调用 `eatASandwich()` 函数。如果抛出的错误与 `SandwichError.outOfCleanDishes` 成员匹配，就调用 `washDishes()` 函数。如果抛出的错误与 `SandwichError.missingIngredients` 成员匹配，就用 `catch` 模式捕获的关联 `[String]` 值调用 `buyGroceries(_:)` 函数。

抛出、捕获和传播错误在 [错误处理](error-handling.md) 中有更详细的介绍。

## 断言和先决条件 {#Assertions-and-Preconditions}

*断言*和*先决条件*是在运行时进行的检查。用它们来确保在执行后续代码之前满足某个必要条件。如果断言或先决条件中的布尔条件求值为 `true`，代码就照常继续执行。如果条件求值为 `false`，程序当前状态就是无效的；代码执行结束，应用被终止。

用断言和先决条件来表达你在编码时所做的假设和期望，从而把它们作为代码的一部分。断言帮助你在开发过程中发现错误和不正确的假设，先决条件帮助你在生产环境中发现问题。

除了在运行时验证你的期望之外，断言和先决条件还成为代码中一种有用的文档形式。与上文 [错误处理](the-basics.md#Error-Handling) 中讨论的错误状况不同，断言和先决条件不用于可恢复的或预期内的错误。因为断言或先决条件失败表示程序状态无效，所以无法捕获失败的断言。从无效状态恢复是不可能的。断言失败时，程序中至少有一项数据无效——但你并不知道它为什么无效，也不知道是否还有其他状态也无效。

使用断言和先决条件并不能代替这样的代码设计：让无效状况不太可能出现。不过，用它们来强制保证数据和状态有效，可以使应用在出现无效状态时更可预测地终止，并有助于让问题更容易调试。如果不检查假设，你可能要过很久，直到代码其他位置开始明显失败、用户数据已被悄悄破坏之后，才会注意到这类问题。一旦检测到无效状态就停止执行，也有助于限制该无效状态造成的损害。

断言和先决条件的区别在于检查的时机：断言只在调试构建中检查，而先决条件在调试构建和生产构建中都会检查。在生产构建中，断言内部的条件不会被求值。这意味着你可以在开发过程中使用任意多的断言，而不会影响生产环境的性能。

### 用断言调试 {#Debugging-with-Assertions}

调用 Swift 标准库中的 [`assert(_:_:file:line:)`](https://developer.apple.com/documentation/swift/assert(_:_:file:line:)) 函数来编写断言。向这个函数传入一个求值为 `true` 或 `false` 的表达式，以及一条在条件结果为 `false` 时显示的消息。例如：

```swift
let age = -3
assert(age >= 0, "A person's age can't be less than zero.")
// This assertion fails because -3 isn't >= 0.
```

在这个例子中，如果 `age >= 0` 求值为 `true`，也就是说如果 `age` 的值非负，代码就继续执行。如果 `age` 的值是负数（如上面的代码所示），那么 `age >= 0` 求值为 `false`，断言失败，终止应用。

可以省略断言消息——例如，当它只是用文字重述该条件时。

```swift
assert(age >= 0)
```

如果代码已经检查了条件，就用 [`assertionFailure(_:file:line:)`](https://developer.apple.com/documentation/swift/assertionfailure(_:file:line:)) 函数表示断言已经失败。例如：

```swift
if age > 10 {
    print("You can ride the roller-coaster or the ferris wheel.")
} else if age >= 0 {
    print("You can ride the ferris wheel.")
} else {
    assertionFailure("A person's age can't be less than zero.")
}
```

### 强制执行先决条件 {#Enforcing-Preconditions}

只要某个条件有可能为假，但为了让代码继续执行又*必须*为真，就应使用先决条件。例如，用先决条件检查下标没有越界，或检查传给函数的值是有效的。

调用 [`precondition(_:_:file:line:)`](https://developer.apple.com/documentation/swift/precondition(_:_:file:line:)) 函数来编写先决条件。向这个函数传入一个求值为 `true` 或 `false` 的表达式，以及一条在条件结果为 `false` 时显示的消息。例如：

```swift
// In the implementation of a subscript...
precondition(index > 0, "Index must be greater than zero.")
```

也可以调用 [`preconditionFailure(_:file:line:)`](https://developer.apple.com/documentation/swift/preconditionfailure(_:file:line:)) 函数来表示已经发生了失败——例如，如果执行了 switch 的 `default` 分支，但所有有效输入数据本应已由 switch 的其他分支之一处理。

> 注意：如果以非检查模式（`-Ounchecked`）编译，先决条件不会被检查。编译器假定先决条件始终为真，并据此优化代码。不过，无论优化设置如何，`fatalError(_:file:line:)` 函数总是会停止执行。
>
> 在原型设计和早期开发阶段，可以用 `fatalError(_:file:line:)` 函数为尚未实现的功能创建占位实现，具体做法是写 `fatalError("Unimplemented")` 作为占位实现。与断言或先决条件不同，致命错误永远不会被优化掉，因此可以确定，执行一旦遇到占位实现就一定会停止。
