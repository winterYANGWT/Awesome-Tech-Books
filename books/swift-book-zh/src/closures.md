# 闭包 {#Closures}

把一起执行的代码组合起来，无需创建具名函数。

*闭包*是自包含的功能块，可以在代码中传递和使用。Swift 中的闭包类似于其他编程语言中的闭包、匿名函数、lambda 和 block。

闭包可以捕获并存储其定义所在上下文中任意常量和变量的引用。这被称为对这些常量和变量进行*闭包捕获*（closing over）。Swift 会替你处理捕获过程中的所有内存管理。

> 注意：如果你不熟悉捕获的概念，不必担心。下文 [捕获值](closures.md#Capturing-Values) 会详细说明。

在 [函数](functions.md) 中介绍的全局函数和嵌套函数，实际上都是闭包的特例。闭包有三种形式：

- 全局函数是有名字、不捕获任何值的闭包。
- 嵌套函数是有名字、可以从其外围函数捕获值的闭包。
- 闭包表达式是用轻量语法书写的匿名闭包，可以从其周围上下文中捕获值。

Swift 的闭包表达式风格简洁清晰，并且带有一些优化，鼓励在常见场景中使用简短、不杂乱的语法。这些优化包括：

- 从上下文推断形参类型和返回值类型
- 单表达式闭包隐式返回
- 简写实参名
- 尾随闭包语法

## 闭包表达式 {#Closure-Expressions}

如 [嵌套函数](functions.md#Nested-Functions) 中所述，嵌套函数是把自包含的代码块命名并定义为更大函数的一部分的便捷方式。不过，有时编写类似函数的构造的更短版本，而不需要完整的声明和名字，会很有用。当你使用把函数作为一个或多个实参的函数或方法时，尤其如此。

*闭包表达式*（closure expression）是用简短、聚焦的语法编写内联闭包的方式。闭包表达式提供了若干语法优化，让你能以缩写形式书写闭包，同时不损失清晰度或意图。下面的闭包表达式示例通过对 `sorted(by:)` 方法的一个示例进行多次改进来展示这些优化，每一次改进都用更简洁的方式表达同样的功能。

### `sorted(by:)` 方法 {#The-Sorted-Method}

Swift 标准库提供了一个名为 `sorted(by:)` 的方法，它根据你提供的排序闭包的输出，对一个元素类型已知的数组进行排序。排序过程完成后，`sorted(by:)` 方法返回一个新数组，其类型和大小与原数组相同，元素按正确的排序顺序排列。原数组不会被 `sorted(by:)` 方法修改。

下面的闭包表达式示例使用 `sorted(by:)` 方法按字母逆序对 `String` 值数组排序。下面是待排序的初始数组：

```swift
let names = ["Chris", "Alex", "Ewa", "Barry", "Daniella"]
```

`sorted(by:)` 方法接受一个闭包，该闭包接受两个与数组内容同类型的实参，并返回一个 `Bool` 值，表示排序后第一个值应出现在第二个值之前还是之后。如果第一个值应出现在第二个值*之前*，排序闭包需要返回 `true`，否则返回 `false`。

本示例对 `String` 值数组排序，因此排序闭包必须是 `(String, String) -> Bool` 类型的函数。

提供排序闭包的一种方式是编写一个类型正确的普通函数，并把它作为实参传给 `sorted(by:)` 方法：

```swift
func backward(_ s1: String, _ s2: String) -> Bool {
    return s1 > s2
}
var reversedNames = names.sorted(by: backward)
// reversedNames is equal to ["Ewa", "Daniella", "Chris", "Barry", "Alex"]
```

如果第一个字符串（`s1`）大于第二个字符串（`s2`），`backward(_:_:)` 函数会返回 `true`，表明在排序后的数组中 `s1` 应出现在 `s2` 之前。对于字符串中的字符，“大于”意味着“在字母表中出现得更靠后”。这意味着字母 `"B"` “大于”字母 `"A"`，字符串 `"Tom"` 大于字符串 `"Tim"`。这会产生按字母逆序的排序，`"Barry"` 排在 `"Alex"` 之前，依此类推。

然而，对于本质上只是单表达式函数（`a > b`）的东西来说，这种写法相当冗长。在本例中，更可取的做法是用闭包表达式语法内联编写排序闭包。

### 闭包表达式语法 {#Closure-Expression-Syntax}

闭包表达式语法的一般形式如下：

```swift
{ (<#parameters#>) -> <#return type#> in
   <#statements#>
}
```

闭包表达式语法中的*形参*可以是输入输出参数，但不能有默认值。如果给可变参数命名，就可以使用可变参数。元组也可以用作形参类型和返回类型。

下面的示例展示了上面 `backward(_:_:)` 函数的闭包表达式版本：

```swift
reversedNames = names.sorted(by: { (s1: String, s2: String) -> Bool in
    return s1 > s2
})
```

注意，这个内联闭包的形参和返回类型的声明与 `backward(_:_:)` 函数中的声明完全相同。两种情况下都写成 `(s1: String, s2: String) -> Bool`。不过，对于内联闭包表达式，形参和返回类型写在花括号*内部*，而不是外部。

闭包主体的开始由 `in` 关键字引入。这个关键字表示闭包的形参和返回类型定义已经结束，闭包主体即将开始。

由于闭包主体很短，甚至可以写在一行里：

```swift
reversedNames = names.sorted(by: { (s1: String, s2: String) -> Bool in return s1 > s2 } )
```

这说明对 `sorted(by:)` 方法的整体调用保持不变。一对圆括号仍然包着该方法的整个实参。不过，这个实参现在是一个内联闭包。

### 从上下文推断类型 {#Inferring-Type-From-Context}

由于排序闭包是作为实参传给方法的，Swift 可以推断其形参的类型以及它返回的值的类型。`sorted(by:)` 方法是在字符串数组上调用的，所以它的实参必须是 `(String, String) -> Bool` 类型的函数。这意味着 `(String, String)` 和 `Bool` 类型不需要写进闭包表达式的定义中。由于所有类型都可以推断出来，返回箭头（`->`）和形参名外面的圆括号也可以省略：

```swift
reversedNames = names.sorted(by: { s1, s2 in return s1 > s2 } )
```

当把闭包作为内联闭包表达式传给函数或方法时，总是可以推断出形参类型和返回类型。因此，当闭包用作函数或方法的实参时，你永远不需要以内联闭包的最完整形式来书写它。

不过，如果你愿意，仍然可以显式写出类型；如果这样做能避免代码读者产生歧义，那么也鼓励这么做。就 `sorted(by:)` 方法而言，由于正在进行排序，闭包的用途很清楚；读者可以安全地假定该闭包很可能在处理 `String` 值，因为它是在协助对字符串数组排序。

### 单表达式闭包的隐式返回 {#Implicit-Returns-from-Single-Expression-Closures}

单表达式闭包可以通过在声明中省略 `return` 关键字来隐式返回其单个表达式的结果，如下面这个前一示例的版本所示：

```swift
reversedNames = names.sorted(by: { s1, s2 in s1 > s2 } )
```

这里，`sorted(by:)` 方法实参的函数类型清楚地表明闭包必须返回一个 `Bool` 值。由于闭包主体只包含一个返回 `Bool` 值的表达式（`s1 > s2`），不存在歧义，因此可以省略 `return` 关键字。

### 简写实参名 {#Shorthand-Argument-Names}

Swift 会自动为内联闭包提供简写实参名，可以用 `$0`、`$1`、`$2` 等名字来引用闭包实参的值。

如果你在闭包表达式中使用这些简写实参名，就可以在定义中省略闭包的实参列表。简写实参名的类型从期望的函数类型推断而来，你使用的编号最大的简写实参决定了闭包接受的实参个数。`in` 关键字也可以省略，因为闭包表达式完全由它的主体组成：

```swift
reversedNames = names.sorted(by: { $0 > $1 } )
```

这里，`$0` 和 `$1` 指代闭包的第一和第二个 `String` 实参。由于 `$1` 是编号最大的简写实参，闭包被理解为接受两个实参。由于这里的 `sorted(by:)` 函数期望一个两个实参都是字符串的闭包，简写实参 `$0` 和 `$1` 的类型都是 `String`。

### 运算符方法 {#Operator-Methods}

实际上，还有一种更*短*的方式来书写上面的闭包表达式。Swift 的 `String` 类型把大于运算符（`>`）针对字符串的实现定义为一个方法，该方法有两个 `String` 类型的形参，并返回 `Bool` 类型的值。这与 `sorted(by:)` 方法所需的方法类型完全匹配。因此，你可以直接传入大于运算符，Swift 会推断你想使用它针对字符串的实现：

```swift
reversedNames = names.sorted(by: >)
```

关于运算符方法的更多内容，参见 [运算符方法](advanced-operators.md#Operator-Methods)。

## 尾随闭包 {#Trailing-Closures}

如果你需要把闭包表达式作为函数的最后一个实参传给函数，而该闭包表达式又很长，那么把它写成*尾随闭包*（trailing closure）可能更方便。尾随闭包写在函数调用的圆括号之后，尽管它仍然是函数的一个实参。使用尾随闭包语法时，不要在函数调用中写出第一个闭包的实参标签。一次函数调用可以包含多个尾随闭包；不过，下面最初的几个示例只使用一个尾随闭包。

```swift
func someFunctionThatTakesAClosure(closure: () -> Void) {
    // function body goes here
}

// Here's how you call this function without using a trailing closure:

someFunctionThatTakesAClosure(closure: {
    // closure's body goes here
})

// Here's how you call this function with a trailing closure instead:

someFunctionThatTakesAClosure() {
    // trailing closure's body goes here
}
```

上面 [闭包表达式语法](closures.md#Closure-Expression-Syntax) 一节中的字符串排序闭包，可以写在 `sorted(by:)` 方法的圆括号之外，作为尾随闭包：

```swift
reversedNames = names.sorted() { $0 > $1 }
```

如果闭包表达式是函数或方法的唯一实参，并且你以尾随闭包的形式提供该表达式，那么在调用函数时，不需要在函数或方法名后写一对圆括号 `()`：

```swift
reversedNames = names.sorted { $0 > $1 }
```

当闭包足够长，无法内联写在一行里时，尾随闭包最为有用。例如，Swift 的 `Array` 类型有一个 `map(_:)` 方法，它接受一个闭包表达式作为唯一实参。该闭包会为数组中的每个元素调用一次，并为该元素返回一个映射后的值（可能是其他类型）。你通过在传给 `map(_:)` 的闭包中编写代码，来指定映射的性质和返回值的类型。

把提供的闭包应用到每个数组元素之后，`map(_:)` 方法返回一个新数组，其中包含所有新的映射值，顺序与它们在原数组中对应值的顺序相同。

下面演示如何使用 `map(_:)` 方法配合尾随闭包，把 `Int` 值数组转换为 `String` 值数组。数组 `[16, 58, 510]` 用于创建新数组 `["OneSix", "FiveEight", "FiveOneZero"]`：

```swift
let digitNames = [
    0: "Zero", 1: "One", 2: "Two",   3: "Three", 4: "Four",
    5: "Five", 6: "Six", 7: "Seven", 8: "Eight", 9: "Nine"
]
let numbers = [16, 58, 510]
```

上面的代码创建了一个字典，把整数数字映射到其英文名称。它还定义了一个整数数组，准备转换为字符串。

现在你可以使用 `numbers` 数组创建一个 `String` 值数组，方法是把闭包表达式作为尾随闭包传给该数组的 `map(_:)` 方法：

```swift
let strings = numbers.map { (number) -> String in
    var number = number
    var output = ""
    repeat {
        output = digitNames[number % 10]! + output
        number /= 10
    } while number > 0
    return output
}
// strings is inferred to be of type [String]
// its value is ["OneSix", "FiveEight", "FiveOneZero"]
```

`map(_:)` 方法会为数组中的每个元素调用一次该闭包表达式。你不需要指定闭包输入形参 `number` 的类型，因为可以从要映射的数组中的值推断出该类型。

在这个示例中，变量 `number` 用闭包 `number` 形参的值初始化，以便能在闭包主体内修改该值。（函数和闭包的形参始终是常量。）该闭包表达式还指定了 `String` 的返回类型，以表明将存储在映射输出数组中的类型。

该闭包表达式每次被调用时都会构建一个名为 `output` 的字符串。它使用求余运算符（`number % 10`）计算 `number` 的最后一位数字，并用这个数字在 `digitNames` 字典中查找相应的字符串。该闭包可用于创建任何大于零的整数的字符串表示。

> 注意：调用 `digitNames` 字典的下标后跟一个感叹号（`!`），因为字典下标返回可选值，以表明如果键不存在，字典查找可能失败。在上面的示例中，可以保证 `number % 10` 始终是 `digitNames` 字典的有效下标键，因此使用感叹号强制解包下标可选返回值中存储的 `String` 值。

从 `digitNames` 字典取回的字符串被添加到 `output` 的*前面*，实际上是在反向构建该数字的字符串版本。（表达式 `number % 10` 对 `16` 得到值 `6`，对 `58` 得到 `8`，对 `510` 得到 `0`。）

然后 `number` 变量除以 `10`。由于它是整数，除法时会向下取整，所以 `16` 变成 `1`，`58` 变成 `5`，`510` 变成 `51`。

重复这个过程，直到 `number` 等于 `0`；此时闭包返回 `output` 字符串，`map(_:)` 方法把它添加到输出数组中。

上面示例中使用尾随闭包语法，把闭包的功能整齐地封装在它所支持的函数之后，而不需要把整个闭包包在 `map(_:)` 方法的外层圆括号内。

如果函数接受多个闭包，你要省略第一个尾随闭包的实参标签，并给其余尾随闭包加标签。例如，下面的函数为照片画廊加载图片：

```swift
func loadPicture(from server: Server, completion: (Picture) -> Void, onFailure: () -> Void) {
    if let picture = download("photo.jpg", from: server) {
        completion(picture)
    } else {
        onFailure()
    }
}
```

调用这个函数加载图片时，你提供两个闭包。第一个闭包是完成处理程序，在下载成功后显示图片。第二个闭包是错误处理程序，向用户显示错误。

```swift
loadPicture(from: someServer) { picture in
    someView.currentPicture = picture
} onFailure: {
    print("Couldn't download the next picture.")
}
```

在这个示例中，`loadPicture(from:completion:onFailure:)` 函数把网络任务分派到后台，并在网络任务完成时调用两个完成处理程序之一。这样编写函数，可以让你把负责处理网络失败的代码与下载成功后更新用户界面的代码清晰地分开，而不用只用一个闭包来处理这两种情况。

> 注意：完成处理程序可能变得难以阅读，尤其是当你必须嵌套多个处理程序时。另一种方法是使用异步代码，详见 [并发](concurrency.md)。

## 捕获值 {#Capturing-Values}

闭包可以从其定义所在的周围上下文中*捕获*常量和变量。然后，闭包可以在其主体内引用并修改这些常量和变量的值，即使定义这些常量和变量的原始作用域已不存在。

在 Swift 中，能够捕获值的闭包最简单的形式是嵌套函数，它写在另一个函数的主体中。嵌套函数可以捕获其外围函数的任意实参，也可以捕获外围函数中定义的任意常量和变量。

下面是一个名为 `makeIncrementer` 的函数的示例，它包含一个名为 `incrementer` 的嵌套函数。嵌套的 `incrementer()` 函数从周围上下文中捕获两个值：`runningTotal` 和 `amount`。捕获这些值之后，`makeIncrementer` 返回 `incrementer` 作为一个闭包，每次调用时把 `runningTotal` 增加 `amount`。

```swift
func makeIncrementer(forIncrement amount: Int) -> () -> Int {
    var runningTotal = 0
    func incrementer() -> Int {
        runningTotal += amount
        return runningTotal
    }
    return incrementer
}
```

`makeIncrementer` 的返回类型是 `() -> Int`。这意味着它返回一个*函数*，而不是一个简单的值。它返回的函数没有形参，每次调用时返回一个 `Int` 值。要了解函数如何返回其他函数，参见 [函数类型作为返回类型](functions.md#Function-Types-as-Return-Types)。

`makeIncrementer(forIncrement:)` 函数定义了一个名为 `runningTotal` 的整数变量，用来存储将要返回的 incrementer 当前的累计总数。该变量初始化为 `0`。

`makeIncrementer(forIncrement:)` 函数有一个 `Int` 形参，实参标签为 `forIncrement`，形参名为 `amount`。传给这个形参的实参值指定了每次调用返回的 incrementer 函数时，`runningTotal` 应增加多少。`makeIncrementer` 函数定义了一个名为 `incrementer` 的嵌套函数，由它执行实际的递增。这个函数只是把 `amount` 加到 `runningTotal` 上，并返回结果。

单独来看，嵌套的 `incrementer()` 函数可能显得不寻常：

```swift
func incrementer() -> Int {
    runningTotal += amount
    return runningTotal
}
```

`incrementer()` 函数没有任何形参，却在函数主体内引用 `runningTotal` 和 `amount`。它通过从外围函数捕获 `runningTotal` 和 `amount` 的*引用*，并在自己的函数主体中使用它们来做到这一点。按引用捕获确保调用 `makeIncrementer` 结束时 `runningTotal` 和 `amount` 不会消失，也确保下次调用 `incrementer` 函数时 `runningTotal` 仍然可用。

> 注意：作为一种优化，如果某个值不会被闭包修改，并且在闭包创建后也不会被修改，Swift 也可能改为捕获并存储该值的*副本*。
>
> Swift 还负责在变量不再需要时释放它们所涉及的全部内存管理。

下面是 `makeIncrementer` 实际运行的示例：

```swift
let incrementByTen = makeIncrementer(forIncrement: 10)
```

这个示例把一个名为 `incrementByTen` 的常量设置为引用一个 incrementer 函数，该函数每次调用时把 `10` 加到它的 `runningTotal` 变量上。多次调用该函数可以展示这一行为：

```swift
incrementByTen()
// returns a value of 10
incrementByTen()
// returns a value of 20
incrementByTen()
// returns a value of 30
```

如果你创建第二个 incrementer，它会有自己存储的引用，指向一个新的、独立的 `runningTotal` 变量：

```swift
let incrementBySeven = makeIncrementer(forIncrement: 7)
incrementBySeven()
// returns a value of 7
```

再次调用原来的 incrementer（`incrementByTen`）会继续递增它自己的 `runningTotal` 变量，不会影响 `incrementBySeven` 捕获的变量：

```swift
incrementByTen()
// returns a value of 40
```

> 注意：如果你把一个闭包赋给某个类实例的属性，而该闭包通过引用该实例或其成员来捕获该实例，就会在闭包和实例之间创建强引用循环。Swift 使用*捕获列表*（capture list）来打破这些强引用循环。更多信息参见 [闭包引起的强引用循环](automatic-reference-counting.md#Strong-Reference-Cycles-for-Closures)。

## 闭包是引用类型 {#Closures-Are-Reference-Types}

在上面的示例中，`incrementBySeven` 和 `incrementByTen` 是常量，但这些常量所指的闭包仍然能够递增它们捕获的 `runningTotal` 变量。这是因为函数和闭包是*引用类型*。

每当你把函数或闭包赋给常量或变量时，实际上是把该常量或变量设为指向该函数或闭包的*引用*。在上面的示例中，保持不变的是 `incrementByTen` *引用*哪个闭包，而不是闭包本身的内容。

这也意味着，如果你把一个闭包赋给两个不同的常量或变量，这两个常量或变量都会引用同一个闭包。

```swift
let alsoIncrementByTen = incrementByTen
alsoIncrementByTen()
// returns a value of 50

incrementByTen()
// returns a value of 60
```

上面的示例表明，调用 `alsoIncrementByTen` 与调用 `incrementByTen` 相同。由于两者引用同一个闭包，它们都会递增并返回同一个累计总数。

## 逃逸闭包 {#Escaping-Closures}

当闭包作为实参传给函数，但在函数返回之后才被调用时，就说这个闭包*逃逸*出了该函数。当你声明一个接受闭包作为其形参之一的函数时，可以在形参类型前写 `@escaping`，表示允许该闭包逃逸。

闭包逃逸的一种方式是把它存储到函数外部定义的变量中。例如，许多启动异步操作的函数会接受一个闭包实参作为完成处理程序。函数在启动操作后返回，但闭包要到操作完成后才被调用——闭包需要逃逸，以便稍后被调用。例如：

```swift
var completionHandlers: [() -> Void] = []
func someFunctionWithEscapingClosure(completionHandler: @escaping () -> Void) {
    completionHandlers.append(completionHandler)
}
```

`someFunctionWithEscapingClosure(_:)` 函数接受一个闭包作为实参，并将其添加到一个在函数外部声明的数组中。如果没有用 `@escaping` 标记这个函数的形参，就会得到编译时错误。

如果 `self` 引用的是类的实例，那么引用 `self` 的逃逸闭包需要特别考虑。在逃逸闭包中捕获 `self` 很容易意外造成强引用循环。关于引用循环的信息，参见 [自动引用计数](automatic-reference-counting.md)。

通常，闭包通过在闭包主体中使用变量来隐式捕获它们，但在这种情况下，你需要显式捕获。如果你想捕获 `self`，在使用它时显式写出 `self`，或者把 `self` 包含在闭包的捕获列表中。显式写出 `self` 可以让你表达自己的意图，并提醒你确认不存在引用循环。例如，在下面的代码中，传给 `someFunctionWithEscapingClosure(_:)` 的闭包显式引用 `self`。相比之下，传给 `someFunctionWithNonescapingClosure(_:)` 的闭包是非逃逸闭包，这意味着它可以隐式引用 `self`。

```swift
func someFunctionWithNonescapingClosure(closure: () -> Void) {
    closure()
}

class SomeClass {
    var x = 10
    func doSomething() {
        someFunctionWithEscapingClosure { self.x = 100 }
        someFunctionWithNonescapingClosure { x = 200 }
    }
}

let instance = SomeClass()
instance.doSomething()
print(instance.x)
// Prints "200".

completionHandlers.first?()
print(instance.x)
// Prints "100".
```

下面是 `doSomething()` 的一个版本，它通过把 `self` 包含在闭包的捕获列表中来捕获 `self`，然后隐式引用 `self`：

```swift
class SomeOtherClass {
    var x = 10
    func doSomething() {
        someFunctionWithEscapingClosure { [self] in x = 100 }
        someFunctionWithNonescapingClosure { x = 200 }
    }
}
```

如果 `self` 是结构体或枚举的实例，你始终可以隐式引用 `self`。不过，当 `self` 是结构体或枚举的实例时，逃逸闭包不能捕获对 `self` 的可变引用。结构体和枚举不允许共享可变性，参见 [结构体和枚举是值类型](classes-and-structures.md#Structures-and-Enumerations-Are-Value-Types)。

```swift
struct SomeStruct {
    var x = 10
    mutating func doSomething() {
        someFunctionWithNonescapingClosure { x = 200 }  // OK
        someFunctionWithEscapingClosure { x = 100 }     // Error
    }
}
```

上面示例中对 `someFunctionWithEscapingClosure` 函数的调用会报错，因为它位于可变方法内部，所以 `self` 是可变的。这违反了结构体的逃逸闭包不能捕获对 `self` 的可变引用这一规则。

## 自动闭包 {#Autoclosures}

*自动闭包*（autoclosure）是一种自动创建的闭包，用于包装作为实参传给函数的表达式。它不接受任何实参，被调用时返回其内部包装的表达式的值。这种语法便利让你可以省略函数形参周围的花括号，只需编写普通表达式，而不用显式写出闭包。

*调用*接受自动闭包的函数很常见，但*实现*这类函数并不常见。例如，`assert(condition:message:file:line:)` 函数的 `condition` 和 `message` 形参都接受自动闭包；它的 `condition` 形参只在调试构建中求值，`message` 形参只在 `condition` 为 `false` 时求值。

自动闭包让你可以延迟求值，因为里面的代码要到你调用闭包时才运行。延迟求值对于有副作用或计算代价高昂的代码很有用，因为它让你可以控制这些代码何时求值。下面的代码展示了闭包如何延迟求值。

```swift
var customersInLine = ["Chris", "Alex", "Ewa", "Barry", "Daniella"]
print(customersInLine.count)
// Prints "5".

let customerProvider = { customersInLine.remove(at: 0) }
print(customersInLine.count)
// Prints "5".

print("Now serving \(customerProvider())!")
// Prints "Now serving Chris!"
print(customersInLine.count)
// Prints "4".
```

尽管闭包内的代码会移除 `customersInLine` 数组的第一个元素，但在闭包实际被调用之前，该数组元素并不会被移除。如果闭包从未被调用，闭包内的表达式就永远不会求值，这意味着该数组元素永远不会被移除。注意，`customerProvider` 的类型不是 `String`，而是 `() -> String`——一个没有形参、返回字符串的函数。

当你把闭包作为实参传给函数时，也会得到同样的延迟求值行为。

```swift
// customersInLine is ["Alex", "Ewa", "Barry", "Daniella"]
func serve(customer customerProvider: () -> String) {
    print("Now serving \(customerProvider())!")
}
serve(customer: { customersInLine.remove(at: 0) } )
// Prints "Now serving Alex!"
```

上面代码清单中的 `serve(customer:)` 函数接受一个返回顾客名字的显式闭包。下面这个 `serve(customer:)` 版本执行同样的操作，但它不是接受显式闭包，而是通过用 `@autoclosure` 特性标记形参类型来接受自动闭包。现在你可以像调用接受 `String` 实参的函数那样调用它，而不是传入闭包。该实参会自动转换为闭包，因为 `customerProvider` 形参的类型标有 `@autoclosure` 特性。

```swift
// customersInLine is ["Ewa", "Barry", "Daniella"]
func serve(customer customerProvider: @autoclosure () -> String) {
    print("Now serving \(customerProvider())!")
}
serve(customer: customersInLine.remove(at: 0))
// Prints "Now serving Ewa!"
```

> 注意：过度使用自动闭包会让代码难以理解。上下文和函数名应能清楚表明求值被延迟了。

如果你想要一个允许逃逸的自动闭包，请同时使用 `@autoclosure` 和 `@escaping` 特性。`@escaping` 特性已在上文 [逃逸闭包](closures.md#Escaping-Closures) 中描述。

```swift
// customersInLine is ["Barry", "Daniella"]
var customerProviders: [() -> String] = []
func collectCustomerProviders(_ customerProvider: @autoclosure @escaping () -> String) {
    customerProviders.append(customerProvider)
}
collectCustomerProviders(customersInLine.remove(at: 0))
collectCustomerProviders(customersInLine.remove(at: 0))

print("Collected \(customerProviders.count) closures.")
// Prints "Collected 2 closures."
for customerProvider in customerProviders {
    print("Now serving \(customerProvider())!")
}
// Prints "Now serving Barry!"
// Prints "Now serving Daniella!"
```

在上面的代码中，`collectCustomerProviders(_:)` 函数没有调用作为 `customerProvider` 实参传给它的闭包，而是把这个闭包追加到 `customerProviders` 数组中。该数组声明在函数作用域之外，这意味着数组中的闭包可以在函数返回后执行。因此，必须允许 `customerProvider` 实参的值逃逸出函数作用域。
