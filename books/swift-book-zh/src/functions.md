# 函数 {#Functions}

定义和调用函数，为实参添加标签，并使用函数的返回值。

*函数*是执行特定任务的自包含代码块。你给函数起一个名字来表明它的作用，需要时用这个名字“调用”函数来执行任务。

Swift 统一的函数语法足够灵活，既能表达没有形参名的简单 C 风格函数，也能表达为每个形参都带名字和实参标签的复杂 Objective-C 风格方法。形参可以提供默认值来简化函数调用，也可以作为输入输出参数传入，在函数执行完毕后修改传入的变量。

Swift 中的每个函数都有类型，由函数的形参类型和返回类型组成。你可以像使用 Swift 中其他任何类型一样使用这个类型，因此很容易把函数作为形参传给其他函数，也可以从函数返回函数。函数还可以写在其他函数内部，把有用的功能封装在嵌套函数的作用域中。

## 定义和调用函数 {#Defining-and-Calling-Functions}

定义函数时，可以选择定义一个或多个有名字、有类型的值作为函数的输入，这些值称为*形参*。你还可以选择定义函数完成后作为输出返回的值的类型，称为函数的*返回类型*。

每个函数都有一个*函数名*，描述函数执行的任务。要使用函数，就用函数名“调用”它，并传入与函数形参类型匹配的输入值（称为*实参*）。函数的实参必须始终按照函数形参列表的顺序提供。

下面示例中的函数叫作 `greet(person:)`，因为它做的就是这件事——它接受一个人的名字作为输入，并返回对这个人的问候。为此，你定义一个输入形参——一个名为 `person` 的 `String` 值——以及一个 `String` 返回类型，用来包含对这个人的问候：

```swift
func greet(person: String) -> String {
    let greeting = "Hello, " + person + "!"
    return greeting
}
```

所有这些信息都汇总在函数的*定义*中，定义以 `func` 关键字开头。你用*返回箭头* `->`（一个连字符后跟一个右尖括号）表示函数的返回类型，后面紧跟要返回的类型的名字。

定义描述了函数做什么、期望接收什么，以及完成后返回什么。定义让函数可以毫无歧义地从代码的其他地方调用：

```swift
print(greet(person: "Anna"))
// Prints "Hello, Anna!"
print(greet(person: "Brian"))
// Prints "Hello, Brian!"
```

你通过在 `person` 实参标签之后传入一个 `String` 值来调用 `greet(person:)` 函数，例如 `greet(person: "Anna")`。因为函数返回 `String` 值，所以可以把 `greet(person:)` 包在 `print(_:separator:terminator:)` 函数的调用中，打印该字符串并查看其返回值，如上所示。

> 注意：`print(_:separator:terminator:)` 函数的第一个实参没有标签，其他实参因为有默认值所以是可选的。这些函数语法的变化会在下文 [函数实参标签和形参名](functions.md#Function-Argument-Labels-and-Parameter-Names) 和 [默认形参值](functions.md#Default-Parameter-Values) 中讨论。

`greet(person:)` 的函数体先定义一个新的 `String` 常量 `greeting`，并把它设成一条简单的问候语。然后使用 `return` 关键字把这条问候语传出函数。在 `return greeting` 这行代码中，函数结束执行并返回 `greeting` 的当前值。

你可以用不同的输入值多次调用 `greet(person:)` 函数。上面的示例展示了用输入值 `"Anna"` 和输入值 `"Brian"` 调用时会发生什么。函数在每种情况下都会返回量身定制的问候语。

为了让这个函数体更短，你可以把创建消息和返回语句合并成一行：

```swift
func greetAgain(person: String) -> String {
    return "Hello again, " + person + "!"
}
print(greetAgain(person: "Anna"))
// Prints "Hello again, Anna!"
```

## 函数形参和返回值 {#Function-Parameters-and-Return-Values}

在 Swift 中，函数形参和返回值非常灵活。你既可以定义只有一个未命名形参的简单工具函数，也可以定义带有富有表现力的形参名和不同形参选项的复杂函数。

### 无形参的函数 {#Functions-Without-Parameters}

函数不是必须定义输入形参。下面是一个没有输入形参的函数，无论何时调用，它总是返回相同的 `String` 消息：

```swift
func sayHelloWorld() -> String {
    return "hello, world"
}
print(sayHelloWorld())
// Prints "hello, world".
```

即使函数不接受任何形参，函数定义中函数名后面仍然需要括号。调用函数时，函数名后面同样跟着一对空括号。

### 有多个形参的函数 {#Functions-With-Multiple-Parameters}

函数可以有多个输入形参，它们写在函数的括号内，以逗号分隔。

这个函数接受一个人的名字以及是否已经问候过这个人作为输入，并返回对这个人的合适问候：

```swift
func greet(person: String, alreadyGreeted: Bool) -> String {
    if alreadyGreeted {
        return greetAgain(person: person)
    } else {
        return greet(person: person)
    }
}
print(greet(person: "Tim", alreadyGreeted: true))
// Prints "Hello again, Tim!"
```

你在括号中同时传入标记为 `person` 的 `String` 实参值和标记为 `alreadyGreeted` 的 `Bool` 实参值，以逗号分隔，来调用 `greet(person:alreadyGreeted:)` 函数。注意，这个函数与前面章节中展示的 `greet(person:)` 函数不同。虽然两个函数的名字都以 `greet` 开头，但 `greet(person:alreadyGreeted:)` 函数接受两个实参，而 `greet(person:)` 函数只接受一个。

### 无返回值的函数 {#Functions-Without-Return-Values}

函数不是必须定义返回类型。下面是 `greet(person:)` 函数的一个版本，它自己打印 `String` 值，而不是返回该值：

```swift
func greet(person: String) {
    print("Hello, \(person)!")
}
greet(person: "Dave")
// Prints "Hello, Dave!"
```

因为它不需要返回值，所以函数定义中不包含返回箭头（`->`）或返回类型。

> 注意：严格来说，这个版本的 `greet(person:)` 函数*仍然*返回一个值，即使没有定义返回值。没有定义返回类型的函数会返回 `Void` 类型的特殊值。这只是一个空元组，写作 `()`。

调用函数时可以忽略函数的返回值：

```swift
func printAndCount(string: String) -> Int {
    print(string)
    return string.count
}
func printWithoutCounting(string: String) {
    let _ = printAndCount(string: string)
}
printAndCount(string: "hello, world")
// prints "hello, world" and returns a value of 12
printWithoutCounting(string: "hello, world")
// prints "hello, world" but doesn't return a value
```

第一个函数 `printAndCount(string:)` 打印一个字符串，然后以 `Int` 类型返回其字符数。第二个函数 `printWithoutCounting(string:)` 调用第一个函数，但忽略它的返回值。调用第二个函数时，消息仍然由第一个函数打印，但返回的值没有被使用。

> 注意：返回值可以被忽略，但声明会返回值的函数必须始终返回值。定义了返回类型的函数不能允许控制流从函数底部流出而不返回值，尝试这样做会导致编译时错误。

### 有多个返回值的函数 {#Functions-with-Multiple-Return-Values}

你可以使用元组类型作为函数的返回类型，以便把多个值作为一个复合返回值的一部分返回。

下面的示例定义了一个名为 `minMax(array:)` 的函数，它找出 `Int` 值数组中的最小值和最大值：

```swift
func minMax(array: [Int]) -> (min: Int, max: Int) {
    var currentMin = array[0]
    var currentMax = array[0]
    for value in array[1..<array.count] {
        if value < currentMin {
            currentMin = value
        } else if value > currentMax {
            currentMax = value
        }
    }
    return (currentMin, currentMax)
}
```

`minMax(array:)` 函数返回一个包含两个 `Int` 值的元组。这两个值标记为 `min` 和 `max`，因此查询函数返回值时可以按名字访问它们。

`minMax(array:)` 的函数体先把两个工作变量 `currentMin` 和 `currentMax` 设为数组中第一个整数的值。然后函数遍历数组中其余的值，逐一检查每个值是否分别小于或大于 `currentMin` 和 `currentMax` 的值。最后，把整体的最小值和最大值作为包含两个 `Int` 值的元组返回。

因为元组的成员值作为函数返回类型的一部分被命名，所以可以用点语法访问它们，取得找到的最小值和最大值：

```swift
let bounds = minMax(array: [8, -6, 2, 109, 3, 71])
print("min is \(bounds.min) and max is \(bounds.max)")
// Prints "min is -6 and max is 109".
```

注意，在从函数返回元组时，元组成员不需要命名，因为它们的名字已经在函数返回类型中指定了。

#### 可选元组返回类型 {#Optional-Tuple-Return-Types}

如果函数要返回的元组类型在整个元组上可能“没有值”，你可以使用*可选*元组返回类型来反映整个元组可以是 `nil` 这一事实。在元组类型的右括号后放置问号，即可写出可选元组返回类型，例如 `(Int, Int)?` 或 `(String, Int, Bool)?`。

> 注意：像 `(Int, Int)?` 这样的可选元组类型不同于包含可选类型的元组，例如 `(Int?, Int?)`。对于可选元组类型，整个元组是可选的，而不只是元组中的每个单独值。

上面的 `minMax(array:)` 函数返回一个包含两个 `Int` 值的元组。然而，该函数没有对它接收的数组做任何安全检查。如果传入的 `array` 实参是空数组，上面定义的 `minMax(array:)` 函数在尝试访问 `array[0]` 时会触发运行时错误。

为了安全地处理空数组，可以编写使用可选元组返回类型的 `minMax(array:)` 函数，并在数组为空时返回 `nil` 值：

```swift
func minMax(array: [Int]) -> (min: Int, max: Int)? {
    if array.isEmpty { return nil }
    var currentMin = array[0]
    var currentMax = array[0]
    for value in array[1..<array.count] {
        if value < currentMin {
            currentMin = value
        } else if value > currentMax {
            currentMax = value
        }
    }
    return (currentMin, currentMax)
}
```

你可以使用可选绑定检查这个版本的 `minMax(array:)` 函数返回的是实际的元组值还是 `nil`：

```swift
if let bounds = minMax(array: [8, -6, 2, 109, 3, 71]) {
    print("min is \(bounds.min) and max is \(bounds.max)")
}
// Prints "min is -6 and max is 109".
```

### 隐式返回的函数 {#Functions-With-an-Implicit-Return}

如果整个函数体是一个表达式，函数会隐式返回该表达式。例如，下面两个函数行为相同：

```swift
func greeting(for person: String) -> String {
    "Hello, " + person + "!"
}
print(greeting(for: "Dave"))
// Prints "Hello, Dave!"

func anotherGreeting(for person: String) -> String {
    return "Hello, " + person + "!"
}
print(anotherGreeting(for: "Dave"))
// Prints "Hello, Dave!"
```

`greeting(for:)` 函数的整个定义就是它返回的问候消息，这意味着它可以使用这种更简短的形式。`anotherGreeting(for:)` 函数使用 `return` 关键字，像更长的函数一样返回相同的问候消息。任何你写成只有一行 `return` 的函数都可以省略 `return`。

正如你将在 [简写 getter 声明](properties.md#Shorthand-Getter-Declaration) 中看到的，属性 getter 也可以使用隐式返回。

> 注意：你写成隐式返回值的代码需要返回某个值。例如，不能把 `print(13)` 用作隐式返回值。不过，可以把 `fatalError("Oh no!")` 这样永不返回的函数用作隐式返回值，因为 Swift 知道隐式返回不会发生。

## 函数实参标签和形参名 {#Function-Argument-Labels-and-Parameter-Names}

每个函数形参都有一个*实参标签*和一个*形参名*。实参标签在调用函数时使用；函数调用中的每个实参前面都写它的实参标签。形参名在函数实现中使用。默认情况下，形参使用自己的形参名作为实参标签。

```swift
func someFunction(firstParameterName: Int, secondParameterName: Int) {
    // In the function body, firstParameterName and secondParameterName
    // refer to the argument values for the first and second parameters.
}
someFunction(firstParameterName: 1, secondParameterName: 2)
```

所有形参必须有唯一的名称。虽然多个形参可以有相同的实参标签，但使用唯一的实参标签有助于让代码更可读。

### 指定实参标签 {#Specifying-Argument-Labels}

你在形参名之前写实参标签，用空格分隔：

```swift
func someFunction(argumentLabel parameterName: Int) {
    // In the function body, parameterName refers to the argument value
    // for that parameter.
}
```

下面是 `greet(person:)` 函数的一个变体，它接受一个人的名字和家乡并返回问候语：

```swift
func greet(person: String, from hometown: String) -> String {
    return "Hello \(person)!  Glad you could visit from \(hometown)."
}
print(greet(person: "Bill", from: "Cupertino"))
// Prints "Hello Bill!  Glad you could visit from Cupertino."
```

使用实参标签可以让函数以富有表现力、类似句子的方式被调用，同时函数体仍然可读且意图清晰。

### 省略实参标签 {#Omitting-Argument-Labels}

如果你不想为某个形参设置实参标签，就在该形参的实参标签位置写一个下划线（`_`）来代替显式实参标签。

```swift
func someFunction(_ firstParameterName: Int, secondParameterName: Int) {
    // In the function body, firstParameterName and secondParameterName
    // refer to the argument values for the first and second parameters.
}
someFunction(1, secondParameterName: 2)
```

如果形参有实参标签，调用函数时该实参*必须*带标签。

### 默认形参值 {#Default-Parameter-Values}

你可以通过在形参类型后面给形参赋值，为函数中的任何形参定义*默认值*。如果定义了默认值，调用函数时可以省略该形参。

```swift
func someFunction(parameterWithoutDefault: Int, parameterWithDefault: Int = 12) {
    // If you omit the second argument when calling this function, then
    // the value of parameterWithDefault is 12 inside the function body.
}
someFunction(parameterWithoutDefault: 3, parameterWithDefault: 6) // parameterWithDefault is 6
someFunction(parameterWithoutDefault: 4) // parameterWithDefault is 12
```

把没有默认值的形参放在函数形参列表的开头，即放在有默认值的形参之前。没有默认值的形参通常对函数含义更重要——把它们写在前面，可以更容易看出调用的是同一个函数，无论是否省略了某些默认形参。

### 可变参数 {#Variadic-Parameters}

*可变参数*接受零个或多个指定类型的值。你使用可变参数来指定调用函数时可以向该形参传入数量可变的输入值。在形参类型名后面插入三个句点字符（`...`）即可写出可变参数。

传给可变参数的值在函数体内以相应类型的数组形式提供。例如，名为 `numbers`、类型为 `Double...` 的可变参数，在函数体内以名为 `numbers`、类型为 `[Double]` 的常量数组形式提供。

下面的示例计算任意长度数字列表的*算术平均值*（也称为*平均数*）：

```swift
func arithmeticMean(_ numbers: Double...) -> Double {
    var total: Double = 0
    for number in numbers {
        total += number
    }
    return total / Double(numbers.count)
}
arithmeticMean(1, 2, 3, 4, 5)
// returns 3.0, which is the arithmetic mean of these five numbers
arithmeticMean(3, 8.25, 18.75)
// returns 10.0, which is the arithmetic mean of these three numbers
```

一个函数可以有多个可变参数。可变参数后面的第一个形参必须有实参标签。实参标签可以明确哪些实参传给可变参数，哪些实参传给可变参数之后的形参。

### 输入输出参数 {#In-Out-Parameters}

函数形参默认是常量。试图在函数体内部更改函数形参的值会导致编译时错误。这意味着你不会误改形参的值。如果你希望函数修改形参的值，并且希望这些修改在函数调用结束后仍然存在，就把该形参定义为*输入输出参数*。

把 `inout` 关键字紧挨着写在形参类型前面，即可写出输入输出参数。输入输出参数有一个值，它被*传入*函数，由函数修改，然后再被*传出*函数，替换原来的值。关于输入输出参数行为及相关编译器优化的详细讨论，参见 [输入输出参数](declarations.md#In-Out-Parameters)。

只能把变量作为输入输出参数的实参传入。不能把常量或字面量作为实参传入，因为常量和字面量不能被修改。把变量作为实参传给输入输出参数时，在变量名前面直接写一个 & 符号（`&`），表示该变量可以被函数修改。

> 注意：输入输出参数不能有默认值，可变参数不能标记为 `inout`。

下面是一个名为 `swapTwoInts(_:_:)` 的函数的示例，它有两个名为 `a` 和 `b` 的输入输出整数形参：

```swift
func swapTwoInts(_ a: inout Int, _ b: inout Int) {
    let temporaryA = a
    a = b
    b = temporaryA
}
```

`swapTwoInts(_:_:)` 函数只是把 `b` 的值换到 `a`，把 `a` 的值换到 `b`。函数执行交换的方式是：把 `a` 的值存入名为 `temporaryA` 的临时常量，把 `b` 的值赋给 `a`，然后把 `temporaryA` 赋给 `b`。

你可以用两个 `Int` 类型的变量调用 `swapTwoInts(_:_:)` 函数来交换它们的值。注意，`someInt` 和 `anotherInt` 传给 `swapTwoInts(_:_:)` 函数时，名字前面带有 & 符号：

```swift
var someInt = 3
var anotherInt = 107
swapTwoInts(&someInt, &anotherInt)
print("someInt is now \(someInt), and anotherInt is now \(anotherInt)")
// Prints "someInt is now 107, and anotherInt is now 3".
```

上面的示例表明，`someInt` 和 `anotherInt` 的原始值被 `swapTwoInts(_:_:)` 函数修改了，尽管它们原本是在函数外部定义的。

> 注意：输入输出参数不同于从函数返回值。上面的 `swapTwoInts` 示例没有定义返回类型，也没有返回值，但它仍然修改了 `someInt` 和 `anotherInt` 的值。输入输出参数是函数在函数体作用域之外产生效果的另一种方式。

## 函数类型 {#Function-Types}

每个函数都有一个特定的*函数类型*，由函数的形参类型和返回类型组成。

例如：

```swift
func addTwoInts(_ a: Int, _ b: Int) -> Int {
    return a + b
}
func multiplyTwoInts(_ a: Int, _ b: Int) -> Int {
    return a * b
}
```

这个示例定义了两个简单的数学函数，名为 `addTwoInts` 和 `multiplyTwoInts`。这两个函数各接受两个 `Int` 值，并返回一个 `Int` 值，即执行相应数学运算的结果。

这两个函数的类型都是 `(Int, Int) -> Int`。可以读作：

“一个有两个形参、两个形参都是 `Int` 类型、并返回 `Int` 类型值的函数。”

下面是另一个示例，是一个没有形参也没有返回值的函数：

```swift
func printHelloWorld() {
    print("hello, world")
}
```

这个函数的类型是 `() -> Void`，或者说“一个没有形参、返回 `Void` 的函数”。

### 使用函数类型 {#Using-Function-Types}

你可以像使用 Swift 中其他任何类型一样使用函数类型。例如，你可以把常量或变量定义为函数类型，并把合适的函数赋给该变量：

```swift
var mathFunction: (Int, Int) -> Int = addTwoInts
```

可以读作：

“定义一个名为 `mathFunction` 的变量，它的类型是‘一个接受两个 `Int` 值并返回 `Int` 值的函数’。把这个新变量设置为引用名为 `addTwoInts` 的函数。”

`addTwoInts(_:_:)` 函数与 `mathFunction` 变量类型相同，因此 Swift 的类型检查器允许这个赋值。

现在你可以用 `mathFunction` 这个名字调用所赋的函数：

```swift
print("Result: \(mathFunction(2, 3))")
// Prints "Result: 5".
```

可以把另一个类型匹配的函数以同样的方式赋给同一个变量，就像非函数类型一样：

```swift
mathFunction = multiplyTwoInts
print("Result: \(mathFunction(2, 3))")
// Prints "Result: 6".
```

与其他任何类型一样，把函数赋给常量或变量时，你可以让 Swift 推断函数类型：

```swift
let anotherMathFunction = addTwoInts
// anotherMathFunction is inferred to be of type (Int, Int) -> Int
```

### 函数类型作为形参类型 {#Function-Types-as-Parameter-Types}

你可以把 `(Int, Int) -> Int` 这样的函数类型用作另一个函数的形参类型。这让你可以把函数实现的某些方面留给函数的调用者在调用函数时提供。

下面是一个打印前面数学函数结果的示例：

```swift
func printMathResult(_ mathFunction: (Int, Int) -> Int, _ a: Int, _ b: Int) {
    print("Result: \(mathFunction(a, b))")
}
printMathResult(addTwoInts, 3, 5)
// Prints "Result: 8".
```

这个示例定义了一个名为 `printMathResult(_:_:_:)` 的函数，它有三个形参。第一个形参名为 `mathFunction`，类型是 `(Int, Int) -> Int`。你可以把该类型的任何函数作为第一个形参的实参传入。第二个和第三个形参名为 `a` 和 `b`，类型都是 `Int`。它们用作所提供的数学函数的两个输入值。

调用 `printMathResult(_:_:_:)` 时，传入 `addTwoInts(_:_:)` 函数以及整数值 `3` 和 `5`。它用值 `3` 和 `5` 调用所提供的函数，并打印结果 `8`。

`printMathResult(_:_:_:)` 的作用是打印调用相应类型的数学函数所得的结果。该函数实现实际做什么并不重要——重要的是函数的类型正确。这使 `printMathResult(_:_:_:)` 能够以类型安全的方式把部分功能交给函数的调用者。

### 函数类型作为返回类型 {#Function-Types-as-Return-Types}

你可以把函数类型用作另一个函数的返回类型。为此，在返回函数的返回箭头（`->`）之后紧接着写一个完整的函数类型。

下一个示例定义了两个简单的函数，名为 `stepForward(_:)` 和 `stepBackward(_:)`。`stepForward(_:)` 函数返回比输入值大 1 的值，`stepBackward(_:)` 函数返回比输入值小 1 的值。两个函数的类型都是 `(Int) -> Int`：

```swift
func stepForward(_ input: Int) -> Int {
    return input + 1
}
func stepBackward(_ input: Int) -> Int {
    return input - 1
}
```

下面是一个名为 `chooseStepFunction(backward:)` 的函数，其返回类型是 `(Int) -> Int`。`chooseStepFunction(backward:)` 函数根据名为 `backward` 的布尔形参返回 `stepForward(_:)` 函数或 `stepBackward(_:)` 函数：

```swift
func chooseStepFunction(backward: Bool) -> (Int) -> Int {
    return backward ? stepBackward : stepForward
}
```

现在你可以使用 `chooseStepFunction(backward:)` 获得一个向某个方向步进的函数：

```swift
var currentValue = 3
let moveNearerToZero = chooseStepFunction(backward: currentValue > 0)
// moveNearerToZero now refers to the stepBackward() function
```

上面的示例判断需要正向还是负向步进，才能让名为 `currentValue` 的变量逐步接近零。`currentValue` 的初始值为 `3`，这意味着 `currentValue > 0` 返回 `true`，使 `chooseStepFunction(backward:)` 返回 `stepBackward(_:)` 函数。返回函数的引用存储在名为 `moveNearerToZero` 的常量中。

既然 `moveNearerToZero` 指向正确的函数，就可以用它来数到零：

```swift
print("Counting to zero:")
// Counting to zero:
while currentValue != 0 {
    print("\(currentValue)... ")
    currentValue = moveNearerToZero(currentValue)
}
print("zero!")
// 3...
// 2...
// 1...
// zero!
```

## 嵌套函数 {#Nested-Functions}

到目前为止本章遇到的所有函数都是*全局函数*的示例，它们定义在全局作用域中。你也可以把函数定义在其他函数的函数体内，称为*嵌套函数*。

嵌套函数默认对外界隐藏，但仍然可以被其外围函数调用和使用。外围函数还可以返回它的某个嵌套函数，以便在另一个作用域中使用该嵌套函数。

你可以重写上面的 `chooseStepFunction(backward:)` 示例，改用并返回嵌套函数：

```swift
func chooseStepFunction(backward: Bool) -> (Int) -> Int {
    func stepForward(input: Int) -> Int { return input + 1 }
    func stepBackward(input: Int) -> Int { return input - 1 }
    return backward ? stepBackward : stepForward
}
var currentValue = -4
let moveNearerToZero = chooseStepFunction(backward: currentValue > 0)
// moveNearerToZero now refers to the nested stepForward() function
while currentValue != 0 {
    print("\(currentValue)... ")
    currentValue = moveNearerToZero(currentValue)
}
print("zero!")
// -4...
// -3...
// -2...
// -1...
// zero!
```
