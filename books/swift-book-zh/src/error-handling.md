# 错误处理 {#Error-Handling}

响应错误并从中恢复。

*错误处理*（error handling）是响应并恢复程序中错误状况的过程。Swift 为在运行时抛出、捕获、传播和操作可恢复错误提供了一流的支持。

某些操作并不保证总能完成执行或产生有用的输出。可选类型用于表示值的缺失，但当操作失败时，弄清失败的原因往往很有用，这样你的代码才能做出相应的响应。

举个例子，考虑从磁盘上的文件读取并处理数据的任务。这个任务可能以多种方式失败，包括文件不存在于指定路径、文件没有读取权限，或者文件没有以兼容的格式编码。区分这些不同的情况，可以让程序解决某些错误，并把无法解决的错误告知用户。

> 注意：Swift 中的错误处理与 Cocoa 和 Objective-C 中使用 `NSError` 类的错误处理模式可以互操作。关于这个类的更多信息，请参阅 [Handling Cocoa Errors in Swift](https://developer.apple.com/documentation/swift/cocoa_design_patterns/handling_cocoa_errors_in_swift)。

## 表示并抛出错误 {#Representing-and-Throwing-Errors}

在 Swift 中，错误由遵循 `Error` 协议的类型的值来表示。这个空协议表明某个类型可以用于错误处理。

Swift 的枚举特别适合为一系列相关的错误状况建模，关联值则允许传递关于错误性质的额外信息。例如，下面是你可能如何表示在游戏里操作自动售货机的错误状况：

```swift
enum VendingMachineError: Error {
    case invalidSelection
    case insufficientFunds(coinsNeeded: Int)
    case outOfStock
}
```

抛出错误让你可以表明发生了意料之外的事情，正常的执行流程无法继续。你使用 `throw` 语句来抛出错误。例如，下面的代码抛出一个错误，表明自动售货机还需要再投入五枚硬币：

```swift
throw VendingMachineError.insufficientFunds(coinsNeeded: 5)
```

## 处理错误 {#Handling-Errors}

错误被抛出时，周围某段代码必须负责处理这个错误——例如，通过修正问题、尝试另一种方法，或者把失败告知用户。

在 Swift 中有四种处理错误的方式。你可以把错误从函数传播到调用该函数的代码，使用 `do`-`catch` 语句处理错误，把错误当作可选值处理，或者断言错误不会发生。下面各节会逐一介绍每种方式。

函数抛出错误时会改变程序的流程，因此能够快速找出代码中可能抛出错误的位置很重要。为了在代码中标识这些位置，在调用可能抛出错误的函数、方法或构造器的代码之前写上 `try` 关键字——或者 `try?`、`try!` 变体。下面各节会介绍这些关键字。

> 注意：Swift 中的错误处理类似于其他语言中的异常处理，同样使用 `try`、`catch` 和 `throw` 关键字。与许多语言（包括 Objective-C）中的异常处理不同，Swift 中的错误处理不涉及展开调用栈——这一过程可能带来可观的计算开销。因此，`throw` 语句的性能特征与 `return` 语句相当。

### 用抛出函数传播错误 {#Propagating-Errors-Using-Throwing-Functions}

要表明一个函数、方法或构造器可以抛出错误，在函数声明的参数之后写上 `throws` 关键字。用 `throws` 标记的函数称为*抛出函数*（throwing function）。如果函数指定了返回类型，就把 `throws` 关键字写在返回箭头（`->`）之前。

```swift
func canThrowErrors() throws -> String

func cannotThrowErrors() -> String
```

抛出函数会把在其内部抛出的错误传播到调用它的作用域。

> 注意：只有抛出函数才能传播错误。非抛出函数内部抛出的任何错误都必须在函数内部处理。

在下面的示例中，`VendingMachine` 类有一个 `vend(itemNamed:)` 方法，如果请求的商品不可用、已售罄，或者价格超过当前投入的金额，它就会抛出相应的 `VendingMachineError`：

```swift
struct Item {
    var price: Int
    var count: Int
}

class VendingMachine {
    var inventory = [
        "Candy Bar": Item(price: 12, count: 7),
        "Chips": Item(price: 10, count: 4),
        "Pretzels": Item(price: 7, count: 11)
    ]
    var coinsDeposited = 0

    func vend(itemNamed name: String) throws {
        guard let item = inventory[name] else {
            throw VendingMachineError.invalidSelection
        }

        guard item.count > 0 else {
            throw VendingMachineError.outOfStock
        }

        guard item.price <= coinsDeposited else {
            throw VendingMachineError.insufficientFunds(coinsNeeded: item.price - coinsDeposited)
        }

        coinsDeposited -= item.price

        var newItem = item
        newItem.count -= 1
        inventory[name] = newItem

        print("Dispensing \(name)")
    }
}
```

`vend(itemNamed:)` 方法的实现使用 `guard` 语句提前退出方法，并在购买零食的任何要求不满足时抛出相应的错误。因为 `throw` 语句会立即转移程序控制权，所以只有所有这些要求都满足时，商品才会被售出。

因为 `vend(itemNamed:)` 方法会传播它抛出的任何错误，所以调用这个方法的任何代码都必须处理这些错误——使用 `do`-`catch` 语句、`try?` 或 `try!`——或者继续传播它们。例如，下面示例中的 `buyFavoriteSnack(person:vendingMachine:)` 也是一个抛出函数，`vend(itemNamed:)` 方法抛出的任何错误都会向上传播到调用 `buyFavoriteSnack(person:vendingMachine:)` 函数的位置。

```swift
let favoriteSnacks = [
    "Alice": "Chips",
    "Bob": "Licorice",
    "Eve": "Pretzels",
]
func buyFavoriteSnack(person: String, vendingMachine: VendingMachine) throws {
    let snackName = favoriteSnacks[person] ?? "Candy Bar"
    try vendingMachine.vend(itemNamed: snackName)
}
```

在这个示例中，`buyFavoriteSnack(person:vendingMachine:)` 函数查找给定人员最喜欢的零食，并通过调用 `vend(itemNamed:)` 方法尝试为他们购买。因为 `vend(itemNamed:)` 方法可能抛出错误，所以调用它时要在前面写上 `try` 关键字。

抛出错误的构造器可以像抛出函数一样传播错误。例如，下面代码中 `PurchasedSnack` 结构体的构造器在构造过程中调用了一个抛出函数，并通过把遇到的任何错误传播给调用者来处理这些错误。

```swift
struct PurchasedSnack {
    let name: String
    init(name: String, vendingMachine: VendingMachine) throws {
        try vendingMachine.vend(itemNamed: name)
        self.name = name
    }
}
```

### 使用 do-catch 处理错误 {#Handling-Errors-Using-Do-Catch}

你使用 `do`-`catch` 语句运行一段代码来处理错误。如果 `do` 子句中的代码抛出错误，就把该错误与 `catch` 子句逐一匹配，以确定其中哪一个能处理这个错误。

下面是 `do`-`catch` 语句的一般形式：

```swift
do {
    try <#expression#>
    <#statements#>
} catch <#pattern 1#> {
    <#statements#>
} catch <#pattern 2#> where <#condition#> {
    <#statements#>
} catch <#pattern 3#>, <#pattern 4#> where <#condition#> {
    <#statements#>
} catch {
    <#statements#>
}
```

在 `catch` 后面写一个模式，以表明该子句能处理哪些错误。如果 `catch` 子句没有模式，该子句就匹配任何错误，并把错误绑定到名为 `error` 的局部常量。关于模式匹配的更多信息，请参阅 [模式](patterns.md)。

例如，下面的代码匹配 `VendingMachineError` 枚举的所有三种情况。

```swift
var vendingMachine = VendingMachine()
vendingMachine.coinsDeposited = 8
do {
    try buyFavoriteSnack(person: "Alice", vendingMachine: vendingMachine)
    print("Success! Yum.")
} catch VendingMachineError.invalidSelection {
    print("Invalid Selection.")
} catch VendingMachineError.outOfStock {
    print("Out of Stock.")
} catch VendingMachineError.insufficientFunds(let coinsNeeded) {
    print("Insufficient funds. Please insert an additional \(coinsNeeded) coins.")
} catch {
    print("Unexpected error: \(error).")
}
// Prints "Insufficient funds. Please insert an additional 2 coins."
```

在上面的示例中，`buyFavoriteSnack(person:vendingMachine:)` 函数在 `try` 表达式中调用，因为它可能抛出错误。如果抛出了错误，执行会立即转移到 `catch` 子句，由这些子句决定是否允许继续传播。如果没有匹配任何模式，错误会被最后的 `catch` 子句捕获，并绑定到局部 `error` 常量。如果没有抛出错误，则执行 `do` 语句中剩余的语句。

`catch` 子句不必处理 `do` 子句中代码可能抛出的每一个错误。如果没有任何 `catch` 子句处理该错误，错误就会传播到周围的作用域。但是，传播出来的错误必须由*某个*周围作用域处理。在非抛出函数中，外层的 `do`-`catch` 语句必须处理该错误。在抛出函数中，要么由外层的 `do`-`catch` 语句处理，要么由调用方处理。如果错误传播到顶层作用域仍未被处理，你就会得到一个运行时错误。

例如，上面的示例可以改写成：任何不是 `VendingMachineError` 的错误都由调用函数捕获：

```swift
func nourish(with item: String) throws {
    do {
        try vendingMachine.vend(itemNamed: item)
    } catch is VendingMachineError {
        print("Couldn't buy that from the vending machine.")
    }
}

do {
    try nourish(with: "Beet-Flavored Chips")
} catch {
    print("Unexpected non-vending-machine-related error: \(error)")
}
// Prints "Couldn't buy that from the vending machine."
```

在 `nourish(with:)` 函数中，如果 `vend(itemNamed:)` 抛出的错误是 `VendingMachineError` 枚举的某个成员，`nourish(with:)` 就通过打印一条消息来处理该错误。否则，`nourish(with:)` 把错误传播到调用它的位置。错误随后会被通用的 `catch` 子句捕获。

另一种捕获若干相关错误的方式是把它们列在 `catch` 之后，用逗号分隔。例如：

```swift
func eat(item: String) throws {
    do {
        try vendingMachine.vend(itemNamed: item)
    } catch VendingMachineError.invalidSelection, VendingMachineError.insufficientFunds, VendingMachineError.outOfStock {
        print("Invalid selection, out of stock, or not enough money.")
    }
}
```

`eat(item:)` 函数列出了要捕获的自动售货机错误，它的错误文本与列表中的项目相对应。如果列出的三种错误中有任何一种被抛出，这个 `catch` 子句就通过打印一条消息来处理它们。其他任何错误都会传播到周围作用域，包括以后可能新增的任何自动售货机错误。

### 把错误转换为可选值 {#Converting-Errors-to-Optional-Values}

你使用 `try?` 把错误转换为可选值来处理错误。如果在求值 `try?` 表达式时抛出了错误，该表达式的值就是 `nil`。例如，在下面的代码中，`x` 和 `y` 的值和行为相同：

```swift
func someThrowingFunction() throws -> Int {
    // ...
}

let x = try? someThrowingFunction()

let y: Int?
do {
    y = try someThrowingFunction()
} catch {
    y = nil
}
```

如果 `someThrowingFunction()` 抛出错误，`x` 和 `y` 的值就是 `nil`。否则，`x` 和 `y` 的值就是该函数返回的值。注意，`x` 和 `y` 是 `someThrowingFunction()` 所返回类型的可选值。这里函数返回整数，所以 `x` 和 `y` 是可选整数。

当你想以相同方式处理所有错误时，使用 `try?` 可以让你写出简洁的错误处理代码。例如，下面的代码使用多种方式获取数据，如果所有方式都失败则返回 `nil`。

```swift
func fetchData() -> Data? {
    if let data = try? fetchDataFromDisk() { return data }
    if let data = try? fetchDataFromServer() { return data }
    return nil
}
```

### 禁用错误传播 {#Disabling-Error-Propagation}

有时你确知某个抛出函数或方法在运行时实际上不会抛出错误。在这种情况下，你可以在表达式前写 `try!` 来禁用错误传播，并把调用包装在一个运行时断言中，断言不会抛出错误。如果确实抛出了错误，你就会得到一个运行时错误。

例如，下面的代码使用 `loadImage(atPath:)` 函数，该函数加载给定路径上的图像资源，或者在无法加载图像时抛出错误。在这个例子中，因为图像随应用一起发布，运行时不会抛出错误，所以适合禁用错误传播。

```swift
let photo = try! loadImage(atPath: "./Resources/John Appleseed.jpg")
```

## 指定错误类型 {#Specifying-the-Error-Type}

上面的所有示例都使用最常见的错误处理方式：代码抛出的错误可以是任何遵循 `Error` 协议的类型的值。这种方式符合现实：你无法预先知道代码运行时可能发生的每一个错误，尤其是在传播其他地方抛出的错误时。它也反映了错误会随时间变化这一事实。库的新版本——包括你的依赖项所使用的库——可能抛出新的错误，而现实世界中用户配置的丰富复杂性可能暴露出开发和测试期间未曾见到的失败模式。上面示例中的错误处理代码总是包含一个默认情况，用来处理没有特定 `catch` 子句的错误。

大多数 Swift 代码不指定所抛错误的类型。不过，在以下特殊情况下，你可能会限制代码只抛出某一种特定类型的错误：

- 在嵌入式系统上运行代码，而该系统不支持动态内存分配时。抛出 `any Error` 实例或其他装箱协议类型的实例，需要在运行时分配内存来存储错误。相比之下，抛出特定类型的错误让 Swift 可以避免为错误进行堆分配。

- 当错误是某段代码（比如库）的实现细节，而不是该代码接口的一部分时。因为这些错误只来自该库，而不来自其他依赖项或该库的客户端，所以你可以为所有可能的失败列出完整清单。而且因为这些错误是该库的实现细节，它们总是在该库内部被处理。

- 在只传播由泛型形参所描述错误的代码中，比如一个接收闭包实参并传播该闭包中任何错误的函数。关于传播特定错误类型与使用 `rethrows` 的对比，请参阅 [重新抛出函数与方法](declarations.md#Rethrowing-Functions-and-Methods)。

例如，考虑一段汇总评分并使用以下错误类型的代码：

```swift
enum StatisticsError: Error {
    case noRatings
    case invalidRating(Int)
}
```

要指定一个函数只抛出 `StatisticsError` 值作为其错误，在声明该函数时写 `throws(StatisticsError)` 而不是只写 `throws`。这种语法也称为*类型化抛出*（typed throws），因为你在声明中把错误类型写在 `throws` 之后。例如，下面的函数抛出 `StatisticsError` 值作为其错误。

```swift
func summarize(_ ratings: [Int]) throws(StatisticsError) {
    guard !ratings.isEmpty else { throw .noRatings }

    var counts = [1: 0, 2: 0, 3: 0]
    for rating in ratings {
        guard rating > 0 && rating <= 3 else { throw .invalidRating(rating) }
        counts[rating]! += 1
    }

    print("*", counts[1]!, "-- **", counts[2]!, "-- ***", counts[3]!)
}
```

在上面的代码中，`summarize(_:)` 函数汇总一组以 1 到 3 为刻度的评分。如果输入无效，该函数会抛出 `StatisticsError` 的实例。上面代码中两处抛出错误的地方都省略了错误类型，因为函数的错误类型已经定义。在像这样的函数中抛出错误时，你可以使用简写 `throw .noRatings`，而不必写 `throw StatisticsError.noRatings`。

当你在函数开头写了特定的错误类型时，Swift 会检查你没有抛出任何其他错误。例如，如果你试图在上面 `summarize(_:)` 函数中使用本章前面示例中的 `VendingMachineError`，那段代码会在编译时产生错误。

你可以在常规抛出函数内部调用使用类型化抛出的函数：

```swift
func someThrowingFunction() throws {
    let ratings = [1, 2, 3, 2, 2, 1]
    try summarize(ratings)
}
```

上面的代码没有为 `someThrowingFunction()` 指定错误类型，所以它抛出 `any Error`。你也可以显式地把错误类型写成 `throws(any Error)`；下面的代码与上面的代码等价：

```swift
func someThrowingFunction() throws(any Error) {
    let ratings = [1, 2, 3, 2, 2, 1]
    try summarize(ratings)
}
```

在这段代码中，`someThrowingFunction()` 传播 `summarize(_:)` 抛出的任何错误。来自 `summarize(_:)` 的错误总是 `StatisticsError` 值，这对 `someThrowingFunction()` 来说也是可以抛出的有效错误。

就像你可以写一个返回类型为 `Never`、永不返回的函数一样，你也可以用 `throws(Never)` 写一个永不抛出错误的函数：

```swift
func nonThrowingFunction() throws(Never) {
  // ...
}
```
这个函数无法抛出错误，因为不可能创建 `Never` 类型的值来抛出。

除了指定函数的错误类型，你还可以为 `do`-`catch` 语句写特定的错误类型。例如：

```swift
let ratings = []
do throws(StatisticsError) {
    try summarize(ratings)
} catch {
    switch error {
    case .noRatings:
        print("No ratings available")
    case .invalidRating(let rating):
        print("Invalid rating: \(rating)")
    }
}
// Prints "No ratings available".
```

在这段代码中，写 `do throws(StatisticsError)` 表明这个 `do`-`catch` 语句抛出 `StatisticsError` 值作为其错误。像其他 `do`-`catch` 语句一样，`catch` 子句可以处理每一个可能的错误，也可以把未处理的错误传播给某个周围作用域去处理。这段代码处理了所有错误，使用一个 `switch` 语句，为每个枚举值列出一个情况。像其他没有模式的 `catch` 子句一样，这个子句匹配任何错误，并把错误绑定到名为 `error` 的局部常量。因为这个 `do`-`catch` 语句抛出 `StatisticsError` 值，所以 `error` 是 `StatisticsError` 类型的值。

上面的 `catch` 子句使用 `switch` 语句来匹配并处理每一个可能的错误。如果你试图给 `StatisticsError` 添加一个新成员却没有更新错误处理代码，Swift 会给出错误，因为 `switch` 语句不再穷尽所有情况。对于捕获自身所有错误的库，你可以使用这种方式来确保任何新错误都有相应的新代码来处理。

如果一个函数或 `do` 块只抛出单一类型的错误，Swift 会推断这段代码使用了类型化抛出。使用这种更简短的语法，你可以把上面的 `do`-`catch` 示例写成如下形式：

```swift
let ratings = []
do {
    try summarize(ratings)
} catch {
    switch error {
    case .noRatings:
        print("No ratings available")
    case .invalidRating(let rating):
        print("Invalid rating: \(rating)")
    }
}
// Prints "No ratings available".
```

尽管上面的 `do`-`catch` 块没有指定它抛出什么类型的错误，Swift 会推断它抛出 `StatisticsError`。你可以显式写上 `throws(any Error)`，以避免让 Swift 推断出类型化抛出。

## 指定清理动作 {#Specifying-Cleanup-Actions}

你使用 `defer` 语句，在代码执行即将离开当前代码块之前执行一组语句。这个语句让你可以执行任何必要的清理，无论执行是*如何*离开当前代码块的——无论是因为抛出错误而离开，还是因为 `return` 或 `break` 之类的语句而离开。例如，你可以使用 `defer` 语句确保文件描述符被关闭、手动分配的内存被释放。

`defer` 语句会把执行推迟到退出当前作用域时。这个语句由 `defer` 关键字和稍后要执行的语句组成。被推迟的语句不能包含任何会把控制权转移出这些语句的代码，例如 `break` 或 `return` 语句，或者抛出错误。被推迟的动作按照与源代码中书写顺序相反的顺序执行。也就是说，第一条 `defer` 语句中的代码最后执行，第二条 `defer` 语句中的代码倒数第二执行，依此类推。源代码顺序中最后一条 `defer` 语句最先执行。

```swift
func processFile(filename: String) throws {
    if exists(filename) {
        let file = open(filename)
        defer {
            close(file)
        }
        while let line = try file.readline() {
            // Work with the file.
        }
        // close(file) is called here, at the end of the scope.
    }
}
```

上面的示例使用 `defer` 语句确保 `open(_:)` 函数有对应的 `close(_:)` 调用。

即使不涉及任何错误处理代码，你也可以使用 `defer` 语句。更多信息请参阅 [延迟执行](control-flow.md#Deferred-Actions)。
