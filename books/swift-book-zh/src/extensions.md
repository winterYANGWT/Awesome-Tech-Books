# 扩展 {#Extensions}

为已有类型添加功能。

*扩展*（extension）可以为已有的类、结构体、枚举或协议类型添加新功能。这包括扩展那些你无法访问其原始源代码的类型的能力，这种能力称为*回溯建模*（retroactive modeling）。扩展与 Objective-C 中的类别（category）类似。（与 Objective-C 的类别不同，Swift 的扩展没有名字。）

Swift 中的扩展可以：

- 添加计算实例属性和计算类型属性
- 定义实例方法和类型方法
- 提供新的构造器
- 定义下标
- 定义并使用新的嵌套类型
- 使已有类型遵循某个协议

在 Swift 中，你甚至可以扩展协议，为其要求提供实现，或者添加遵循类型可以利用的额外功能。更多细节详见 [协议扩展](protocols.md#Protocol-Extensions)。

> 注意：扩展可以为类型添加新功能，但不能重写已有的功能。

## 扩展语法 {#Extension-Syntax}

用 `extension` 关键字声明扩展：

```swift
extension SomeType {
    // new functionality to add to SomeType goes here
}
```

扩展可以扩展已有类型，使其采纳一个或多个协议。要添加协议一致性，协议的书写方式与为类或结构体书写时相同：

```swift
extension SomeType: SomeProtocol, AnotherProtocol {
    // implementation of protocol requirements goes here
}
```

以这种方式添加协议一致性在 [用扩展添加协议一致性](protocols.md#Adding-Protocol-Conformance-with-an-Extension) 中有描述。

扩展可以用来扩展已有的泛型类型，详见 [扩展泛型类型](generics.md#Extending-a-Generic-Type)。你还可以扩展泛型类型，以有条件地添加功能，详见 [带泛型 where 子句的扩展](generics.md#Extensions-with-a-Generic-Where-Clause)。

> 注意：如果你用扩展为已有类型添加新功能，那么该类型的所有现有实例都可以使用这些新功能，即使这些实例是在定义扩展之前创建的。

## 计算属性 {#Computed-Properties}

扩展可以为已有类型添加计算实例属性和计算类型属性。下面的例子为 Swift 内置的 `Double` 类型添加了五个计算实例属性，为处理距离单位提供基本支持：

```swift
extension Double {
    var km: Double { return self * 1_000.0 }
    var m: Double { return self }
    var cm: Double { return self / 100.0 }
    var mm: Double { return self / 1_000.0 }
    var ft: Double { return self / 3.28084 }
}
let oneInch = 25.4.mm
print("One inch is \(oneInch) meters")
// Prints "One inch is 0.0254 meters".
let threeFeet = 3.ft
print("Three feet is \(threeFeet) meters")
// Prints "Three feet is 0.914399970739201 meters".
```

这些计算属性表示一个 `Double` 值应被视为某种长度单位。虽然它们实现为计算属性，但这些属性的名字可以通过点语法附加到浮点字面量值后面，以此用该字面量值执行距离换算。

在本例中，`Double` 值 `1.0` 被视为表示“一米”。这就是 `m` 计算属性返回 `self` 的原因——表达式 `1.m` 被认为计算出一个值为 `1.0` 的 `Double` 值。

其他单位需要经过一些换算才能表示成以米为单位的值。一公里等于 1,000 米，所以 `km` 计算属性把值乘以 `1_000.00`，换算成以米表示的数。类似地，一米等于 3.28084 英尺，所以 `ft` 计算属性把底层的 `Double` 值除以 `3.28084`，把它从英尺换算成米。

这些属性是只读计算属性，因此为了简洁，它们没有写 `get` 关键字。它们的返回值是 `Double` 类型，可以在任何接受 `Double` 的数学计算中使用：

```swift
let aMarathon = 42.km + 195.m
print("A marathon is \(aMarathon) meters long")
// Prints "A marathon is 42195.0 meters long".
```

> 注意：扩展可以添加新的计算属性，但不能添加存储属性，也不能为已有属性添加属性观察器。

## 构造器 {#Initializers}

扩展可以为已有类型添加新的构造器。这让你可以扩展其他类型，使其接受你自己的自定义类型作为构造器参数，或者提供该类型原始实现中未包含的额外构造选项。

扩展可以为类添加新的便利构造器，但不能为类添加新的指定构造器或析构器。指定构造器和析构器必须始终由类的原始实现提供。

如果你用扩展为一个值类型添加构造器，而该值类型为其所有存储属性都提供了默认值，并且没有定义任何自定义构造器，那么你可以在扩展的构造器中调用该值类型的默认构造器和成员逐一构造器。如果你把构造器写在值类型的原始实现中，就不会有这种行为，详见 [值类型的构造器代理](initialization.md#Initializer-Delegation-for-Value-Types)。

如果你用扩展为一个在另一个模块中声明的结构体添加构造器，那么在调用定义模块中的某个构造器之前，新构造器不能访问 `self`。

下面的例子定义了一个自定义的 `Rect` 结构体来表示几何矩形。这个例子还定义了两个辅助结构体 `Size` 和 `Point`，它们都为所有属性提供了默认值 `0.0`：

```swift
struct Size {
    var width = 0.0, height = 0.0
}
struct Point {
    var x = 0.0, y = 0.0
}
struct Rect {
    var origin = Point()
    var size = Size()
}
```

因为 `Rect` 结构体为其所有属性提供了默认值，所以它会自动获得一个默认构造器和一个成员逐一构造器，详见 [默认构造器](initialization.md#Default-Initializers)。这些构造器可用于创建新的 `Rect` 实例：

```swift
let defaultRect = Rect()
let memberwiseRect = Rect(origin: Point(x: 2.0, y: 2.0),
    size: Size(width: 5.0, height: 5.0))
```

你可以扩展 `Rect` 结构体，提供一个额外的构造器，接受特定的中心点和尺寸：

```swift
extension Rect {
    init(center: Point, size: Size) {
        let originX = center.x - (size.width / 2)
        let originY = center.y - (size.height / 2)
        self.init(origin: Point(x: originX, y: originY), size: size)
    }
}
```

这个新构造器首先根据提供的 `center` 点和 `size` 值计算出合适的原点。然后，该构造器调用结构体自动生成的成员逐一构造器 `init(origin:size:)`，把新的 origin 和 size 值存入相应的属性：

```swift
let centerRect = Rect(center: Point(x: 4.0, y: 4.0),
    size: Size(width: 3.0, height: 3.0))
// centerRect's origin is (2.5, 2.5) and its size is (3.0, 3.0)
```

> 注意：如果你用扩展提供了新构造器，你仍然要负责确保每个实例在构造器完成后都完全初始化。

## 方法 {#Methods}

扩展可以为已有类型添加新的实例方法和类型方法。下面的例子为 `Int` 类型添加了一个名为 `repetitions` 的新实例方法：

```swift
extension Int {
    func repetitions(task: () -> Void) {
        for _ in 0..<self {
            task()
        }
    }
}
```

`repetitions(task:)` 方法接受一个类型为 `() -> Void` 的实参，表示一个没有形参也不返回值的函数。

定义这个扩展之后，你可以在任何整数上调用 `repetitions(task:)` 方法来执行指定次数的任务：

```swift
3.repetitions {
    print("Hello!")
}
// Hello!
// Hello!
// Hello!
```

### 可变实例方法 {#Mutating-Instance-Methods}

用扩展添加的实例方法也可以修改（即*改变*）实例本身。修改 `self` 或其属性的结构体和枚举方法必须把该实例方法标记为 `mutating`，就像原始实现中的可变方法一样。

下面的例子为 Swift 的 `Int` 类型添加了一个名为 `square` 的新可变方法，它对原始值求平方：

```swift
extension Int {
    mutating func square() {
        self = self * self
    }
}
var someInt = 3
someInt.square()
// someInt is now 9
```

## 下标 {#Subscripts}

扩展可以为已有类型添加新的下标。这个例子为 Swift 内置的 `Int` 类型添加一个整数下标。这个下标 `[n]` 返回该数字从右往左数第 `n` 位的十进制数字：

- `123456789[0]` 返回 `9`
- `123456789[1]` 返回 `8`

……以此类推：

```swift
extension Int {
    subscript(digitIndex: Int) -> Int {
        var decimalBase = 1
        for _ in 0..<digitIndex {
            decimalBase *= 10
        }
        return (self / decimalBase) % 10
    }
}
746381295[0]
// returns 5
746381295[1]
// returns 9
746381295[2]
// returns 2
746381295[8]
// returns 7
```

如果 `Int` 值的位数不足以支持所请求的索引，下标实现会返回 `0`，就好像这个数已在左侧用零补齐一样：

```swift
746381295[9]
// returns 0, as if you had requested:
0746381295[9]
```

## 嵌套类型 {#Nested-Types}

扩展可以为已有的类、结构体和枚举添加新的嵌套类型：

```swift
extension Int {
    enum Kind {
        case negative, zero, positive
    }
    var kind: Kind {
        switch self {
        case 0:
            return .zero
        case let x where x > 0:
            return .positive
        default:
            return .negative
        }
    }
}
```

这个例子为 `Int` 添加了一个新的嵌套枚举。这个名为 `Kind` 的枚举表示某个特定整数所代表的数的种类。具体来说，它表示该数是负数、零还是正数。

这个例子还为 `Int` 添加了一个新的计算实例属性 `kind`，它返回该整数对应的 `Kind` 枚举成员。

现在，这个嵌套枚举可以与任何 `Int` 值一起使用：

```swift
func printIntegerKinds(_ numbers: [Int]) {
    for number in numbers {
        switch number.kind {
        case .negative:
            print("- ", terminator: "")
        case .zero:
            print("0 ", terminator: "")
        case .positive:
            print("+ ", terminator: "")
        }
    }
    print("")
}
printIntegerKinds([3, 19, -27, 0, -6, 0, 7])
// Prints "+ + - 0 - 0 + ".
```

这个函数 `printIntegerKinds(_:)` 接受一个 `Int` 值组成的输入数组，并依次遍历这些值。对于数组中的每个整数，函数会考虑该整数的 `kind` 计算属性，并打印相应的描述。

> 注意：`number.kind` 已知是 `Int.Kind` 类型。因此，所有 `Int.Kind` 的成员值都可以在 `switch` 语句中写成简写形式，例如用 `.negative` 而不是 `Int.Kind.negative`。
