# 不透明类型与装箱协议类型 {#Opaque-and-Boxed-Protocol-Types}

隐藏有关值类型的实现细节。

Swift 提供了两种隐藏值类型细节的方式：不透明类型和装箱协议类型。隐藏类型信息在模块与调用该模块的代码之间的边界上很有用，因为返回值的底层类型可以保持私有。

返回不透明类型的函数或方法会隐藏其返回值的类型信息。它不把具体类型用作函数的返回类型，而是用返回值所支持的协议来描述它。不透明类型会保留类型标识——编译器可以访问类型信息，但模块的客户端不能。

装箱协议类型可以存储任何遵循指定协议的类型的实例。装箱协议类型不保留类型标识——值的具体类型要到运行时才可知，而且随着存入不同的值，它还可能随时间变化。

## 不透明类型解决的问题 {#The-Problem-that-Opaque-Types-Solve}

例如，假设你正在编写一个绘制 ASCII 艺术图形的模块。ASCII 艺术图形的基本特征是有一个 `draw()` 函数，它返回该图形的字符串表示；你可以把它作为 `Shape` 协议的要求：

```swift
protocol Shape {
    func draw() -> String
}

struct Triangle: Shape {
    var size: Int
    func draw() -> String {
       var result: [String] = []
       for length in 1...size {
           result.append(String(repeating: "*", count: length))
       }
       return result.joined(separator: "\n")
    }
}
let smallTriangle = Triangle(size: 3)
print(smallTriangle.draw())
// *
// **
// ***
```

你可以用泛型来实现诸如将图形垂直翻转之类的操作，如下面的代码所示。然而，这种方法有一个重要的局限：翻转后的结果会暴露用于创建它的确切泛型类型。

```swift
struct FlippedShape<T: Shape>: Shape {
    var shape: T
    func draw() -> String {
        let lines = shape.draw().split(separator: "\n")
        return lines.reversed().joined(separator: "\n")
    }
}
let flippedTriangle = FlippedShape(shape: smallTriangle)
print(flippedTriangle.draw())
// ***
// **
// *
```

如果像下面代码那样定义 `JoinedShape<T: Shape, U: Shape>` 结构体来垂直拼接两个图形，那么把三角形与翻转后的三角形拼接起来，就会得到 `JoinedShape<Triangle, FlippedShape<Triangle>>` 这样的类型。

```swift
struct JoinedShape<T: Shape, U: Shape>: Shape {
    var top: T
    var bottom: U
    func draw() -> String {
       return top.draw() + "\n" + bottom.draw()
    }
}
let joinedTriangles = JoinedShape(top: smallTriangle, bottom: flippedTriangle)
print(joinedTriangles.draw())
// *
// **
// ***
// ***
// **
// *
```

暴露关于图形创建过程的详细信息，会使得那些本不该成为 ASCII 艺术模块公共接口一部分的类型，由于需要写出完整的返回类型而泄漏出去。模块内部的代码可以用多种方式构建同一个图形，而模块外部使用该图形的其他代码不应该需要操心一连串变换的实现细节。像 `JoinedShape` 和 `FlippedShape` 这样的包装类型对模块的使用者来说并不重要，也不应该可见。模块的公共接口由拼接、翻转图形之类的操作组成，这些操作返回另一个 `Shape` 值。

## 返回不透明类型 {#Returning-an-Opaque-Type}

你可以把不透明类型看作泛型类型的反面。泛型类型让调用函数的代码为该函数的形参和返回值挑选类型，而函数实现无需关心具体类型。例如，下面代码中的函数返回的类型取决于它的调用者：

```swift
func max<T>(_ x: T, _ y: T) -> T where T: Comparable { ... }
```

调用 `max(_:_:)` 的代码为 `x` 和 `y` 选择值，而这两个值的类型决定了 `T` 的具体类型。调用代码可以使用任何遵循 `Comparable` 协议的类型。函数内部的代码以通用方式编写，因此能处理调用者提供的任何类型。`max(_:_:)` 的实现只使用所有 `Comparable` 类型共有的功能。

对于带不透明返回类型的函数，这些角色正好反过来。不透明类型让函数实现为它返回的值挑选类型，而调用该函数的代码无需关心具体类型。例如，下面示例中的函数返回一个梯形，但不暴露该图形的底层类型。

```swift
struct Square: Shape {
    var size: Int
    func draw() -> String {
        let line = String(repeating: "*", count: size)
        let result = Array<String>(repeating: line, count: size)
        return result.joined(separator: "\n")
    }
}

func makeTrapezoid() -> some Shape {
    let top = Triangle(size: 2)
    let middle = Square(size: 2)
    let bottom = FlippedShape(shape: top)
    let trapezoid = JoinedShape(
        top: top,
        bottom: JoinedShape(top: middle, bottom: bottom)
    )
    return trapezoid
}
let trapezoid = makeTrapezoid()
print(trapezoid.draw())
// *
// **
// **
// **
// **
// *
```

这个例子中的 `makeTrapezoid()` 函数把返回类型声明为 `some Shape`；因此，该函数会返回某个给定类型的值，该类型遵循 `Shape` 协议，但不指明任何具体类型。以这种方式编写 `makeTrapezoid()`，可以让它表达其公共接口的基本方面——它返回的值是一个图形——而不让构成该图形的具体类型成为其公共接口的一部分。这个实现使用了两个三角形和一个正方形，但该函数可以改写为以多种其他方式绘制梯形，而不改变其返回类型。

这个例子突出了不透明返回类型如何类似于泛型类型的反面。`makeTrapezoid()` 内部的代码可以返回它需要的任何类型，只要该类型遵循 `Shape` 协议，就像泛型函数的调用代码那样。调用该函数的代码需要像泛型函数的实现那样以通用方式编写，这样才能处理 `makeTrapezoid()` 返回的任何 `Shape` 值。

你还可以把不透明返回类型与泛型结合起来。下面代码中的两个函数都返回某个遵循 `Shape` 协议的类型的值。

```swift
func flip<T: Shape>(_ shape: T) -> some Shape {
    return FlippedShape(shape: shape)
}
func join<T: Shape, U: Shape>(_ top: T, _ bottom: U) -> some Shape {
    JoinedShape(top: top, bottom: bottom)
}

let opaqueJoinedTriangles = join(smallTriangle, flip(smallTriangle))
print(opaqueJoinedTriangles.draw())
// *
// **
// ***
// ***
// **
// *
```

本例中 `opaqueJoinedTriangles` 的值与本章前面 [不透明类型解决的问题](opaque-types.md#The-Problem-that-Opaque-Types-Solve) 一节的泛型示例中的 `joinedTriangles` 相同。然而，与那个示例中的值不同，`flip(_:)` 和 `join(_:_:)` 会把泛型图形操作返回的底层类型包装在不透明返回类型中，从而避免这些类型暴露出来。这两个函数都是泛型的，因为它们依赖的类型是泛型的，而且函数的类型形参传递了 `FlippedShape` 和 `JoinedShape` 所需的类型信息。

如果带不透明返回类型的函数从多个位置返回，那么所有可能的返回值都必须具有相同的类型。对于泛型函数，该返回类型可以使用函数的泛型类型形参，但它仍然必须是单一类型。例如，下面是一个*无效*的图形翻转函数版本，它包含一个针对正方形的特殊情形：

```swift
func invalidFlip<T: Shape>(_ shape: T) -> some Shape {
    if shape is Square {
        return shape // Error: Return types don't match.
    }
    return FlippedShape(shape: shape) // Error: Return types don't match.
}
```

如果你用 `Square` 调用这个函数，它会返回 `Square`；否则返回 `FlippedShape`。这违反了只能返回单一类型值的要求，使 `invalidFlip(_:)` 成为无效代码。修复 `invalidFlip(_:)` 的一种方法是将正方形的特殊情形移入 `FlippedShape` 的实现中，这样该函数就总是返回 `FlippedShape` 值：

```swift
struct FlippedShape<T: Shape>: Shape {
    var shape: T
    func draw() -> String {
        if shape is Square {
           return shape.draw()
        }
        let lines = shape.draw().split(separator: "\n")
        return lines.reversed().joined(separator: "\n")
    }
}
```

始终返回单一类型的要求并不妨碍你在不透明返回类型中使用泛型。下面是一个函数的示例，它把自己的类型形参纳入所返回值的底层类型中：

```swift
func `repeat`<T: Shape>(shape: T, count: Int) -> some Collection {
    return Array<T>(repeating: shape, count: count)
}
```

在这种情况下，返回值的底层类型会随 `T` 变化：无论传入什么图形，`repeat(shape:count:)` 都会创建并返回该图形的数组。不过，返回值的底层类型始终是相同的 `[T]`，因此它遵守了带不透明返回类型的函数只能返回单一类型值这一要求。

## 装箱协议类型 {#Boxed-Protocol-Types}

装箱协议类型有时也称为*存在类型*（existential type），这一名称来自“存在某个类型 *T*，使得 *T* 遵循该协议”这一说法。要创建装箱协议类型，请在协议名称前写 `any`。下面是一个例子：

```swift
struct VerticalShapes: Shape {
    var shapes: [any Shape]
    func draw() -> String {
        return shapes.map { $0.draw() }.joined(separator: "\n\n")
    }
}

let largeTriangle = Triangle(size: 5)
let largeSquare = Square(size: 5)
let vertical = VerticalShapes(shapes: [largeTriangle, largeSquare])
print(vertical.draw())
```

在上面的例子中，`VerticalShapes` 把 `shapes` 的类型声明为 `[any Shape]`——一个由装箱的 `Shape` 元素组成的数组。数组中的每个元素可以是不同的类型，而且这些类型都必须遵循 `Shape` 协议。为了支持这种运行时的灵活性，Swift 在必要时会增加一层间接，这一间接层称为*装箱*（box），它会带来性能开销。

在 `VerticalShapes` 类型内部，代码可以使用 `Shape` 协议要求的方法、属性和下标。例如，`VerticalShapes` 的 `draw()` 方法会对数组中的每个元素调用 `draw()` 方法。这个方法之所以可用，是因为 `Shape` 要求一个 `draw()` 方法。相反，尝试访问三角形的 `size` 属性，或访问 `Shape` 未要求的任何其他属性或方法，都会产生错误。

对比一下 `shapes` 可以使用的三种类型：

- 使用泛型，即编写 `struct VerticalShapes<S: Shape>` 和 `var shapes: [S]`，会创建一个数组，其元素是某种特定的图形类型，并且该特定类型的标识对与数组交互的任何代码都是可见的。

- 使用不透明类型，即编写 `var shapes: [some Shape]`，会创建一个数组，其元素是某种特定的图形类型，并且该特定类型的标识是隐藏的。

- 使用装箱协议类型，即编写 `var shapes: [any Shape]`，会创建一个可以存储不同类型元素的数组，并且这些类型的标识是隐藏的。

在这种情况下，装箱协议类型是唯一能让 `VerticalShapes` 的调用者混用不同种类图形的方法。

当你知道装箱值的底层类型时，可以使用 `as` 转换。例如：

```swift
if let downcastTriangle = vertical.shapes[0] as? Triangle {
    print(downcastTriangle.size)
}
// Prints "5".
```

更多信息，参见 [向下转型](type-casting.md#Downcasting)。

## 不透明类型与装箱协议类型的区别 {#Differences-Between-Opaque-Types-and-Boxed-Protocol-Types}

返回不透明类型看起来与把装箱协议类型用作函数的返回类型非常相似，但这两类返回类型的区别在于是否保留类型标识。不透明类型指的是某一个特定类型，尽管函数的调用者无法看到是哪个类型；装箱协议类型则可以指任何遵循该协议的类型。一般来说，装箱协议类型在它们存储的值的底层类型方面给你更多灵活性，而不透明类型让你能对这些底层类型做出更强的保证。

例如，下面是 `flip(_:)` 的一个版本，它使用装箱协议类型而不是不透明返回类型作为返回类型：

```swift
func protoFlip<T: Shape>(_ shape: T) -> Shape {
    return FlippedShape(shape: shape)
}
```

这个版本的 `protoFlip(_:)` 与 `flip(_:)` 具有相同的函数体，并且总是返回相同类型的值。与 `flip(_:)` 不同，`protoFlip(_:)` 返回的值不要求始终具有相同的类型——它只需遵循 `Shape` 协议。换句话说，`protoFlip(_:)` 与其调用者订立的 API 约定比 `flip(_:)` 宽松得多。它保留了返回多种类型值的灵活性：

```swift
func protoFlip<T: Shape>(_ shape: T) -> Shape {
    if shape is Square {
        return shape
    }

    return FlippedShape(shape: shape)
}
```

修改后的代码版本会根据传入的图形返回 `Square` 实例或 `FlippedShape` 实例。这个函数返回的两个翻转后的图形可能具有完全不同的类型。该函数的其他有效版本在翻转同一图形的多个实例时，可能返回不同类型的值。`protoFlip(_:)` 提供的较不具体的返回类型信息意味着，许多依赖类型信息的操作无法用于返回的值。例如，无法编写一个 `==` 运算符来比较这个函数返回的结果。

```swift
let protoFlippedTriangle = protoFlip(smallTriangle)
let sameThing = protoFlip(smallTriangle)
protoFlippedTriangle == sameThing  // Error
```

示例最后一行出现错误有几个原因。直接的问题是 `Shape` 没有把 `==` 运算符作为其协议要求的一部分。如果你尝试添加一个这样的运算符，接下来会遇到的问题是 `==` 运算符需要知道其左操作数和右操作数的类型。这类运算符通常接受 `Self` 类型的实参，匹配任何采纳该协议的具体类型，但给协议添加 `Self` 要求后，就无法容纳把协议当作类型使用时发生的类型擦除。

把装箱协议类型用作函数的返回类型，让你可以灵活地返回任何遵循该协议的类型。然而，这种灵活性的代价是某些操作无法用于返回的值。示例展示了 `==` 运算符如何不可用——它依赖于特定的类型信息，而使用装箱协议类型不会保留这些信息。

这种方法的另一个问题是图形变换无法嵌套。翻转一个三角形的结果是 `Shape` 类型的值，而 `protoFlip(_:)` 函数接受某种遵循 `Shape` 协议的类型的实参。然而，装箱协议类型的值并不遵循该协议；`protoFlip(_:)` 返回的值不遵循 `Shape`。这意味着像 `protoFlip(protoFlip(smallTriangle))` 这样应用多次变换的代码是无效的，因为翻转后的图形不是 `protoFlip(_:)` 的有效实参。

相比之下，不透明类型会保留底层类型的标识。Swift 可以推断关联类型，这让你能在装箱协议类型无法用作返回值的地方使用不透明返回值。例如，下面是 [泛型](generics.md) 中 `Container` 协议的一个版本：

```swift
protocol Container {
    associatedtype Item
    var count: Int { get }
    subscript(i: Int) -> Item { get }
}
extension Array: Container { }
```

你不能把 `Container` 用作函数的返回类型，因为该协议有关联类型。你也不能把它用作泛型返回类型中的约束，因为在函数体之外没有足够的信息来推断泛型类型应该是什么。

```swift
// Error: Protocol with associated types can't be used as a return type.
func makeProtocolContainer<T>(item: T) -> Container {
    return [item]
}

// Error: Not enough information to infer C.
func makeProtocolContainer<T, C: Container>(item: T) -> C {
    return [item]
}
```

使用不透明类型 `some Container` 作为返回类型，可以表达所需的 API 约定——函数返回一个容器，但不指明该容器的类型：

```swift
func makeOpaqueContainer<T>(item: T) -> some Container {
    return [item]
}
let opaqueContainer = makeOpaqueContainer(item: 12)
let twelve = opaqueContainer[0]
print(type(of: twelve))
// Prints "Int".
```

`twelve` 的类型被推断为 `Int`，这说明了类型推断对不透明类型同样有效。在 `makeOpaqueContainer(item:)` 的实现中，这个不透明容器的底层类型是 `[T]`。在这里，`T` 是 `Int`，所以返回值是一个整数数组，`Item` 关联类型被推断为 `Int`。`Container` 上的下标返回 `Item`，这意味着 `twelve` 的类型也被推断为 `Int`。

## 不透明参数类型 {#Opaque-Parameter-Types}

除了用 `some` 返回不透明类型之外，你还可以在函数、下标或构造器的形参类型中写 `some`。不过，在形参类型中写 `some` 只是泛型的一种更简短的语法，而不是不透明类型。例如，下面两个函数是等价的：

```swift
func drawTwiceGeneric<SomeShape: Shape>(_ shape: SomeShape) -> String {
    let drawn = shape.draw()
    return drawn + "\n" + drawn
}

func drawTwiceSome(_ shape: some Shape) -> String {
    let drawn = shape.draw()
    return drawn + "\n" + drawn
}
```

`drawTwiceGeneric(_:)` 函数声明了一个名为 `SomeShape` 的泛型类型形参，并带有要求 `SomeShape` 遵循 `Shape` 协议的约束。`drawTwiceSome(_:)` 函数把 `some Shape` 用作其实参的类型。这会为该函数创建一个新的、未命名的泛型类型形参，并带有要求该类型遵循 `Shape` 协议的约束。由于这个泛型类型没有名字，你无法在函数的其他地方引用该类型。

如果你在多个形参的类型前写 `some`，这些泛型类型各自独立。例如：

```swift
func combine(shape s1: some Shape, with s2: some Shape) -> String {
    return s1.draw() + "\n" + s2.draw()
}

combine(smallTriangle, trapezoid)
```

在 `combine(shape:with:)` 函数中，第一个和第二个形参的类型都必须遵循 `Shape` 协议，但并没有任何约束要求它们为同一类型。调用 `combine(shape:with)` 时，你可以传入两个不同的图形——在这里是一个三角形和一个梯形。

与 [泛型](generics.md) 一章中描述的具名泛型类型形参语法不同，这种轻量语法不能包含泛型 `where` 子句或任何同类型（`==`）约束。此外，对于非常复杂的约束，使用这种轻量语法可能会难以阅读。
