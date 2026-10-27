# 类型 {#Types}

使用内置的具名类型和复合类型。

Swift 中的类型分为两类：具名类型和复合类型。*具名类型*（named type）是定义时可以指定特定名称的类型。具名类型包括类、结构体、枚举和协议。例如，名为 `MyClass` 的用户自定义类的实例，其类型就是 `MyClass`。除了用户自定义的具名类型，Swift 标准库还定义了许多常用的具名类型，包括表示数组、字典和可选值的类型。

在其他语言中通常被视为基本类型或原始类型的数据类型——例如表示数字、字符和字符串的类型——实际上是具名类型，由 Swift 标准库使用结构体定义和实现。由于它们是具名类型，你可以用扩展声明来扩展它们的行为以满足程序的需要，详见 [扩展](extensions.md) 和 [扩展声明](declarations.md#Extension-Declaration)。

*复合类型*（compound type）是没有名称、由 Swift 语言本身定义的类型。复合类型有两种：函数类型和元组类型。复合类型可以包含具名类型和其他复合类型。例如，元组类型 `(Int, (Int, Int))` 包含两个元素：第一个是具名类型 `Int`，第二个是另一个复合类型 `(Int, Int)`。

你可以用圆括号包住具名类型或复合类型。然而，在类型外面加圆括号没有任何作用。例如，`(Int)` 等价于 `Int`。

本章讨论 Swift 语言本身定义的类型，并说明 Swift 的类型推断行为。

> 类型语法：
>
> *type* → *function-type* \
> *type* → *array-type* \
> *type* → *dictionary-type* \
> *type* → *type-identifier* \
> *type* → *tuple-type* \
> *type* → *optional-type* \
> *type* → *implicitly-unwrapped-optional-type* \
> *type* → *protocol-composition-type* \
> *type* → *opaque-type* \
> *type* → *boxed-protocol-type* \
> *type* → *metatype-type* \
> *type* → *any-type* \
> *type* → *self-type* \
> *type* → **`(`** *type* **`)`**

## 类型标注 {#Type-Annotation}

*类型标注*（type annotation）显式指定变量或表达式的类型。类型标注以冒号（`:`）开头，以类型结尾，如下例所示：

```swift
let someTuple: (Double, Double) = (3.14159, 2.71828)
func someFunction(a: Int) { /* ... */ }
```

在第一个例子中，表达式 `someTuple` 被指定为元组类型 `(Double, Double)`。在第二个例子中，函数 `someFunction` 的形参 `a` 被指定为 `Int` 类型。

类型标注可以在类型前面包含一个可选的类型特性列表。

> 类型标注语法：
>
> *type-annotation* → **`:`** *attributes*_?_ *type*

## 类型标识符 {#Type-Identifier}

*类型标识符*（type identifier）指代具名类型，或者具名类型或复合类型的类型别名。

大多数情况下，类型标识符直接指代与其名称相同的具名类型。例如，`Int` 是一个类型标识符，直接指代具名类型 `Int`；类型标识符 `Dictionary<String, Int>` 直接指代具名类型 `Dictionary<String, Int>`。

有两种情况下，类型标识符指代的类型与其名称不同。第一种情况，类型标识符指代具名类型或复合类型的类型别名。例如，在下面的例子中，类型标注中使用的 `Point` 指代元组类型 `(Int, Int)`。

```swift
typealias Point = (Int, Int)
let origin: Point = (0, 0)
```

第二种情况，类型标识符使用点（`.`）语法来指代在其他模块中声明或嵌套在其他类型中的具名类型。例如，下面代码中的类型标识符引用了在 `ExampleModule` 模块中声明的具名类型 `MyType`。

```swift
var someValue: ExampleModule.MyType
```

> 类型标识符语法：
>
> *type-identifier* → *type-name* *generic-argument-clause*_?_ | *type-name* *generic-argument-clause*_?_ **`.`** *type-identifier* \
> *type-name* → *identifier*

## 元组类型 {#Tuple-Type}

*元组类型*（tuple type）是由圆括号括起来的、以逗号分隔的类型列表。

你可以把元组类型用作函数的返回类型，使函数能够返回包含多个值的单个元组。你也可以给元组类型的元素命名，并用这些名称来引用各个元素的值。元素名称由一个标识符后紧跟一个冒号（:）组成。同时演示这两个特性的示例，请参阅 [有多个返回值的函数](functions.md#Functions-with-Multiple-Return-Values)。

当元组类型的某个元素有名称时，该名称就是该类型的一部分。

```swift
var someTuple = (top: 10, bottom: 12)  // someTuple is of type (top: Int, bottom: Int)
someTuple = (top: 4, bottom: 42) // OK: Names match.
someTuple = (9, 99)              // OK: Names are inferred.
someTuple = (left: 5, right: 5)  // Error: Names don't match.
```

除 `Void` 外，所有元组类型都包含两个或更多类型；`Void` 是空元组类型 `()` 的类型别名。

> 元组类型语法：
>
> *tuple-type* → **`(`** **`)`** | **`(`** *tuple-type-element* **`,`** *tuple-type-element-list* **`)`** \
> *tuple-type-element-list* → *tuple-type-element* | *tuple-type-element* **`,`** *tuple-type-element-list* \
> *tuple-type-element* → *element-name* *type-annotation* | *type* \
> *element-name* → *identifier*

## 函数类型 {#Function-Type}

*函数类型*（function type）表示函数、方法或闭包的类型，由形参类型和返回类型组成，两者之间用箭头（`->`）分隔：

```swift
(<#parameter type#>) -> <#return type#>
```

*形参类型*是以逗号分隔的类型列表。因为*返回类型*可以是元组类型，所以函数类型支持返回多个值的函数和方法。

函数类型 `() -> T`（其中 `T` 是任意类型）的形参可以应用 `autoclosure` 特性，在调用处隐式创建闭包。这样可以在语法上方便地延迟表达式的求值，而无需在调用函数时编写显式闭包。自动闭包函数类型形参的示例，请参阅 [自动闭包](closures.md#Autoclosures)。

函数类型的*形参类型*中可以包含可变参数。在语法上，可变参数由一个基本类型名后紧跟三个点（`...`）组成，例如 `Int...`。可变参数被当作一个数组来处理，数组元素就是该基本类型名所表示的类型。例如，可变参数 `Int...` 被当作 `[Int]`。使用可变参数的示例，请参阅 [可变参数](functions.md#Variadic-Parameters)。

要指定输入输出参数，请在形参类型前加上 `inout` 关键字。你不能用 `inout` 关键字标记可变参数或返回类型。输入输出参数在 [输入输出参数](functions.md#In-Out-Parameters) 中讨论。

如果函数类型只有一个形参，而该形参的类型是元组类型，那么在书写该函数的类型时必须给这个元组类型加上圆括号。例如，`((Int, Int)) -> Void` 是一个接收单个 `(Int, Int)` 元组类型形参且不返回任何值的函数类型。相比之下，如果不加圆括号，`(Int, Int) -> Void` 是一个接收两个 `Int` 形参且不返回任何值的函数类型。同样，由于 `Void` 是 `()` 的类型别名，函数类型 `(Void) -> Void` 与 `(()) -> ()` 相同——后者是一个接收单个空元组实参的函数。这些类型与 `() -> ()` 不同——后者是一个不接收任何实参的函数。

函数和方法中的实参名称不是相应函数类型的一部分。例如：

```swift
func someFunction(left: Int, right: Int) {}
func anotherFunction(left: Int, right: Int) {}
func functionWithDifferentLabels(top: Int, bottom: Int) {}

var f = someFunction // The type of f is (Int, Int) -> Void, not (left: Int, right: Int) -> Void.
f = anotherFunction              // OK
f = functionWithDifferentLabels  // OK

func functionWithDifferentArgumentTypes(left: Int, right: String) {}
f = functionWithDifferentArgumentTypes     // Error

func functionWithDifferentNumberOfArguments(left: Int, right: Int, top: Int) {}
f = functionWithDifferentNumberOfArguments // Error
```

由于实参标签不是函数类型的一部分，因此在书写函数类型时应当省略它们。

```swift
var operation: (lhs: Int, rhs: Int) -> Int     // Error
var operation: (_ lhs: Int, _ rhs: Int) -> Int // OK
var operation: (Int, Int) -> Int               // OK
```

如果一个函数类型包含多个箭头（`->`），这些函数类型会从右向左分组。例如，函数类型 `(Int) -> (Int) -> Int` 被理解为 `(Int) -> ((Int) -> Int)`——也就是说，一个接收 `Int` 并返回另一个函数的函数，后者接收并返回 `Int`。

可以抛出或重新抛出错误的函数，其函数类型必须包含 `throws` 关键字。你可以在 `throws` 后面的圆括号中指定一个类型，以指明函数抛出的错误类型。抛出的错误类型必须遵循 `Error` 协议。只写 `throws` 而不指定类型，等同于写 `throws(any Error)`。省略 `throws` 等同于写 `throws(Never)`。函数抛出的错误类型可以是任何遵循 `Error` 的类型，包括泛型类型、装箱协议类型和不透明类型。

函数抛出的错误类型是该函数类型的一部分，而错误类型之间的子类型关系意味着相应的函数类型之间也具有子类型关系。例如，如果你声明一个自定义的 `MyError` 类型，那么某些函数类型之间的关系如下，从超类型到子类型：

1. 抛出任意错误的函数，标记为 `throws(any Error)`
1. 抛出特定错误的函数，标记为 `throws(MyError)`
1. 不抛出错误的函数，标记为 `throws(Never)`

这些子类型关系带来以下结果：

- 你可以在使用抛出函数的地方使用不抛出的函数。
- 你可以在使用抛出函数的地方使用抛出具体错误类型的函数。
- 你可以在使用抛出更一般错误类型的函数的地方使用抛出更具体错误类型的函数。

如果你在函数类型中使用关联类型或泛型类型形参作为抛出的错误类型，那么该关联类型或泛型类型形参会被隐式要求遵循 `Error` 协议。

抛出函数和重新抛出函数在 [抛出函数与方法](declarations.md#Throwing-Functions-and-Methods) 和 [重新抛出函数与方法](declarations.md#Rethrowing-Functions-and-Methods) 中描述。

异步函数的函数类型必须用 `async` 关键字标记。`async` 关键字是函数类型的一部分，同步函数是异步函数的子类型。因此，你可以在使用异步函数的地方使用同步函数。关于异步函数的信息，请参阅 [异步函数和方法](declarations.md#Asynchronous-Functions-and-Methods)。

### 非逃逸闭包的限制 {#Restrictions-for-Nonescaping-Closures}

非逃逸函数类型的形参不能被存储到 `Any` 类型的属性、变量或常量中，因为那可能让该值逃逸。

非逃逸函数类型的形参不能作为实参传给另一个非逃逸函数形参。这一限制有助于 Swift 在编译期而非运行期完成更多针对内存访问冲突的检查。例如：

```swift
let external: (() -> Void) -> Void = { _ in () }
func takesTwoFunctions(first: (() -> Void) -> Void, second: (() -> Void) -> Void) {
    first { first {} }       // Error
    second { second {}  }    // Error

    first { second {} }      // Error
    second { first {} }      // Error

    first { external {} }    // OK
    external { first {} }    // OK
}
```

在上面的代码中，`takesTwoFunctions(first:second:)` 的两个形参都是函数。两个形参都没有标记 `@escaping`，因此它们都是非逃逸的。

上例中标记为“Error”的四个函数调用会导致编译错误。由于 `first` 和 `second` 这两个形参是非逃逸函数，它们不能作为实参传给另一个非逃逸函数形参。相比之下，标记为“OK”的两个函数调用不会导致编译错误。这些函数调用没有违反限制，因为 `external` 不是 `takesTwoFunctions(first:second:)` 的形参之一。

如果你需要避开这一限制，可以把其中一个形参标记为逃逸，或者使用 `withoutActuallyEscaping(_:do:)` 函数，把某个非逃逸函数形参临时转换为逃逸函数。关于如何避免内存访问冲突，请参阅 [内存安全](memory-safety.md)。

> 函数类型语法：
>
> *function-type* → *attributes*_?_ *function-type-argument-clause* **`async`**_?_ *throws-clause*_?_ **`->`** *type*
>
> *function-type-argument-clause* → **`(`** **`)`** \
> *function-type-argument-clause* → **`(`** *function-type-argument-list* **`...`**_?_ **`)`**
>
> *function-type-argument-list* → *function-type-argument* | *function-type-argument* **`,`** *function-type-argument-list* \
> *function-type-argument* → *attributes*_?_ *parameter-modifier*_?_ *type* | *argument-label* *type-annotation* \
> *argument-label* → *identifier*
>
> *throws-clause* → **`throws`** | **`throws`** **`(`** *type* **`)`**

## 数组类型 {#Array-Type}

Swift 语言为 Swift 标准库的 `Array<Element>` 类型提供了以下语法糖：

```swift
[<#type#>]
```

也就是说，下面两个声明是等价的：

```swift
let someArray: Array<String> = ["Alex", "Brian", "Dave"]
let someArray: [String] = ["Alex", "Brian", "Dave"]
```

在这两种情况下，常量 `someArray` 都被声明为字符串数组。可以通过下标访问数组的元素，即在方括号中指定一个有效的索引值：`someArray[0]` 指的是索引为 0 的元素，即 `"Alex"`。

你可以通过嵌套方括号来创建多维数组，其中元素基本类型的名称写在最内层的方括号中。例如，你可以用三对方括号创建一个三维整数数组：

```swift
var array3D: [[[Int]]] = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
```

访问多维数组中的元素时，最左边的下标索引指的是最外层数组中该索引处的元素。向右的下一个下标索引指的是向内嵌套一层的数组中该索引处的元素。依此类推。这意味着在上面的例子中，`array3D[0]` 指的是 `[[1, 2], [3, 4]]`，`array3D[0][1]` 指的是 `[3, 4]`，`array3D[0][1][1]` 指的是值 4。

关于 Swift 标准库 `Array` 类型的详细讨论，请参阅 [数组](collection-types.md#Arrays)。

> 数组类型语法：
>
> *array-type* → **`[`** *type* **`]`**

## 字典类型 {#Dictionary-Type}

Swift 语言为 Swift 标准库的 `Dictionary<Key, Value>` 类型提供了以下语法糖：

```swift
[<#key type#>: <#value type#>]
```

也就是说，下面两个声明是等价的：

```swift
let someDictionary: [String: Int] = ["Alex": 31, "Paul": 39]
let someDictionary: Dictionary<String, Int> = ["Alex": 31, "Paul": 39]
```

在这两种情况下，常量 `someDictionary` 都被声明为以字符串为键、以整数为值的字典。

可以通过下标访问字典的值，即在方括号中指定相应的键：`someDictionary["Alex"]` 指的是与键 `"Alex"` 关联的值。下标返回字典值类型的可选值。如果指定的键不在字典中，下标会返回 `nil`。

字典的键类型必须遵循 Swift 标准库的 `Hashable` 协议。

关于 Swift 标准库 `Dictionary` 类型的详细讨论，请参阅 [字典](collection-types.md#Dictionaries)。

> 字典类型语法：
>
> *dictionary-type* → **`[`** *type* **`:`** *type* **`]`**

## 可选类型 {#Optional-Type}

Swift 语言把后缀 `?` 定义为具名类型 `Optional<Wrapped>` 的语法糖，该类型定义在 Swift 标准库中。也就是说，下面两个声明是等价的：

```swift
var optionalInteger: Int?
var optionalInteger: Optional<Int>
```

在这两种情况下，变量 `optionalInteger` 都被声明为可选整数类型。注意，类型和 `?` 之间不能出现空白。

类型 `Optional<Wrapped>` 是一个有两个成员 `none` 和 `some(Wrapped)` 的枚举，这两个成员用于表示可能存在也可能不存在的值。任何类型都可以被显式声明为（或隐式转换为）可选类型。如果你在声明可选变量或属性时没有提供初始值，它的值会自动默认为 `nil`。

如果可选类型的实例包含值，你可以使用后缀运算符 `!` 访问该值，如下所示：

```swift
optionalInteger = 42
optionalInteger! // 42
```

使用 `!` 运算符解包一个值为 `nil` 的可选值会导致运行时错误。

你也可以使用可选链和可选绑定，有条件地对可选表达式执行操作。如果值为 `nil`，则不执行任何操作，因此不会产生运行时错误。

关于更多信息以及展示如何使用可选类型的示例，请参阅 [可选类型](the-basics.md#Optionals)。

> 可选类型语法：
>
> *optional-type* → *type* **`?`**

## 隐式解包可选类型 {#Implicitly-Unwrapped-Optional-Type}

Swift 语言把后缀 `!` 定义为具名类型 `Optional<Wrapped>`（定义在 Swift 标准库中）的语法糖，并附加了在访问时自动解包的行为。如果你试图使用一个值为 `nil` 的隐式解包可选值，就会遇到运行时错误。除了隐式解包这一行为之外，下面两个声明是等价的：

```swift
var implicitlyUnwrappedString: String!
var explicitlyUnwrappedString: Optional<String>
```

注意，类型和 `!` 之间不能出现空白。

由于隐式解包会改变包含该类型的声明的含义，所以嵌套在元组类型或泛型类型内部的可选类型——例如字典或数组的元素类型——不能被标记为隐式解包。例如：

```swift
let tupleOfImplicitlyUnwrappedElements: (Int!, Int!)  // Error
let implicitlyUnwrappedTuple: (Int, Int)!             // OK

let arrayOfImplicitlyUnwrappedElements: [Int!]        // Error
let implicitlyUnwrappedArray: [Int]!                  // OK
```

由于隐式解包可选类型与可选值具有相同的 `Optional<Wrapped>` 类型，因此你可以在代码中所有能使用可选值的地方使用隐式解包可选类型。例如，你可以把隐式解包可选类型的值赋给可选类型的变量、常量和属性，反之亦然。

与可选类型一样，如果你在声明隐式解包可选变量或属性时没有提供初始值，它的值会自动默认为 `nil`。

使用可选链，有条件地对隐式解包可选表达式执行操作。如果值为 `nil`，则不执行任何操作，因此不会产生运行时错误。

关于隐式解包可选类型的更多信息，请参阅 [隐式解包可选类型](the-basics.md#Implicitly-Unwrapped-Optionals)。

> 隐式解包可选类型语法：
>
> *implicitly-unwrapped-optional-type* → *type* **`!`**

## 协议组合类型 {#Protocol-Composition-Type}

*协议组合类型*（protocol composition type）定义的类型遵循一组指定协议中的每一个协议，或者既是给定类的子类、又遵循一组指定协议中的每一个协议。协议组合类型只能用于在类型标注、泛型形参子句和泛型 `where` 子句中指定类型。

协议组合类型的形式如下：

```swift
<#Protocol 1#> & <#Protocol 2#>
```

协议组合类型允许你指定一个值，其类型遵循多个协议的要求，而无需显式定义一个新的具名协议来继承你希望该类型遵循的每一个协议。例如，你可以使用协议组合类型 `ProtocolA & ProtocolB & ProtocolC`，而不必声明一个新的协议去继承 `ProtocolA`、`ProtocolB` 和 `ProtocolC`。同样，你可以使用 `SuperClass & ProtocolA`，而不必声明一个新的协议，让它成为 `SuperClass` 的子类并遵循 `ProtocolA`。

协议组合列表中的每一项都是以下之一；该列表最多只能包含一个类：

- 类的名称
- 协议的名称
- 一个类型别名，其底层类型是协议组合类型、协议或类。

当协议组合类型包含类型别名时，同一个协议可能在定义中出现多次——重复项会被忽略。例如，下面代码中 `PQR` 的定义等价于 `P & Q & R`。

```swift
typealias PQ = P & Q
typealias PQR = PQ & Q & R
```

> 协议组合类型语法：
>
> *protocol-composition-type* → *type-identifier* **`&`** *protocol-composition-continuation* \
> *protocol-composition-continuation* → *type-identifier* | *protocol-composition-type*

## 不透明类型 {#Opaque-Type}

*不透明类型*（opaque type）定义一个遵循某个协议或协议组合的类型，但不指定底层的具体类型。

不透明类型可以作为函数或下标的返回类型出现，作为函数、下标或构造器的形参类型出现，或作为属性的类型出现。不透明类型不能作为元组类型或泛型类型的一部分出现，例如数组的元素类型或可选值的被包装类型。

不透明类型的形式如下：

```swift
some <#constraint#>
```

*约束*是类类型、协议类型、协议组合类型或 `Any`。只有当某个值是遵循所列协议或协议组合、或继承自所列类的类型的实例时，这个值才能被用作该不透明类型的实例。与不透明值交互的代码，只能以*约束*所定义的接口允许的方式使用该值。

在编译期，类型为不透明类型的值有一个具体的底层类型，Swift 可以利用该底层类型进行优化。然而，不透明类型形成了一道边界，关于该底层类型的信息无法越过它。

协议声明不能包含不透明类型。类不能把不透明类型用作非 final 方法的返回类型。

使用不透明类型作为返回类型的函数，必须返回共享同一个底层类型的值。返回类型可以包含属于该函数泛型类型形参的类型。例如，函数 `someFunction<T>()` 可以返回 `T` 类型或 `Dictionary<String, T>` 类型的值。

为形参书写不透明类型，是使用泛型类型但不为泛型类型形参指定名称的语法糖。这个隐式泛型类型形参带有一个约束，要求它遵循不透明类型中所指定的协议。如果你写了多个不透明类型，每个都会产生自己的泛型类型形参。例如，下面两个声明是等价的：

```swift
func someFunction(x: some MyProtocol, y: some MyProtocol) { }
func someFunction<T1: MyProtocol, T2: MyProtocol>(x: T1, y: T2) { }
```

在第二个声明中，由于泛型类型形参 `T1` 和 `T2` 有名称，你可以在代码的其他地方引用这些类型。相比之下，第一个声明中的泛型类型形参没有名称，无法在其他代码中被引用。

不能在可变参数的类型中使用不透明类型。

不能把不透明类型用作所返回函数类型的形参，也不能用作本身是函数类型的形参类型中的形参。在这些位置，函数的调用方将不得不构造一个该未知类型的值。

```swift
protocol MyProtocol { }
func badFunction() -> (some MyProtocol) -> Void { }  // Error
func anotherBadFunction(callback: (some MyProtocol) -> Void) { }  // Error
```

> 不透明类型语法：
>
> *opaque-type* → **`some`** *type*

## 装箱协议类型 {#Boxed-Protocol-Type}

*装箱协议类型*（boxed protocol type）定义一个遵循某个协议或协议组合的类型，并且遵循该协议的具体类型可以在程序运行期间变化。

装箱协议类型的形式如下：

```swift
any <#constraint#>
```

*约束*是协议类型、协议组合类型、协议类型的元类型或协议组合类型的元类型。

在运行期，装箱协议类型的实例可以包含任何满足*约束*的类型的值。这一行为与不透明类型的工作方式形成对比：对于不透明类型，在编译期已知某个具体的遵循类型。处理装箱协议类型时所使用的额外一层间接称为*装箱*（boxing）。装箱通常需要单独分配内存来存储，并需要额外一层间接来访问，这会在运行期带来性能开销。

对 `Any` 或 `AnyObject` 类型应用 `any` 没有效果，因为这些类型已经是装箱协议类型。

> 装箱协议类型语法：
>
> *boxed-protocol-type* → **`any`** *type*

## 元类型 {#Metatype-Type}

*元类型*（metatype type）指代任意类型的类型，包括类类型、结构体类型、枚举类型和协议类型。

类、结构体或枚举类型的元类型是该类型的名称后跟 `.Type`。协议类型的元类型——而不是运行期遵循该协议的具体类型——是该协议的名称后跟 `.Protocol`。例如，类类型 `SomeClass` 的元类型是 `SomeClass.Type`，协议 `SomeProtocol` 的元类型是 `SomeProtocol.Protocol`。

你可以使用后缀 `self` 表达式来把类型作为值访问。例如，`SomeClass.self` 返回 `SomeClass` 本身，而不是 `SomeClass` 的实例。而 `SomeProtocol.self` 返回 `SomeProtocol` 本身，而不是运行期遵循 `SomeProtocol` 的某个类型的实例。你可以把某个类型的实例传给 `type(of:)` 函数，以该实例的动态运行期类型作为值来访问，如下例所示：

```swift
class SomeBaseClass {
    class func printClassName() {
        print("SomeBaseClass")
    }
}
class SomeSubClass: SomeBaseClass {
    override class func printClassName() {
        print("SomeSubClass")
    }
}
let someInstance: SomeBaseClass = SomeSubClass()
// The compile-time type of someInstance is SomeBaseClass,
// and the runtime type of someInstance is SomeSubClass
type(of: someInstance).printClassName()
// Prints "SomeSubClass".
```

更多信息请参阅 Swift 标准库中的 [`type(of:)`](https://developer.apple.com/documentation/swift/type(of:))。

使用构造器表达式，可以从某个类型的元类型值构造该类型的实例。对于类的实例，所调用的构造器必须用 `required` 关键字标记，或者整个类必须用 `final` 关键字标记。

```swift
class AnotherSubClass: SomeBaseClass {
    let string: String
    required init(string: String) {
        self.string = string
    }
    override class func printClassName() {
        print("AnotherSubClass")
    }
}
let metatype: AnotherSubClass.Type = AnotherSubClass.self
let anotherInstance = metatype.init(string: "some string")
```

> 元类型语法：
>
> *metatype-type* → *type* **`.`** **`Type`** | *type* **`.`** **`Protocol`**

## Any 类型 {#Any-Type}

`Any` 类型可以包含来自所有其他类型的值。`Any` 可以用作以下任意类型的实例的具体类型：

- 类、结构体或枚举
- 元类型，例如 `Int.self`
- 元素为任意类型的元组
- 闭包或函数类型

```swift
let mixed: [Any] = ["one", 2, true, (4, 5.3), { () -> Int in return 6 }]
```

当你把 `Any` 用作实例的具体类型时，需要先把该实例转换为已知类型，才能访问它的属性或方法。具体类型为 `Any` 的实例会保留其原始动态类型，并且可以使用类型转换运算符之一——`as`、`as?` 或 `as!`——转换为该类型。例如，使用 `as?` 把异构数组中的第一个对象有条件地向下转型为 `String`，如下所示：

```swift
if let first = mixed.first as? String {
    print("The first item, '\(first)', is a string.")
}
// Prints "The first item, 'one', is a string."
```

关于类型转换的更多信息，请参阅 [类型转换](type-casting.md)。

`AnyObject` 协议类似于 `Any` 类型。所有类都隐式遵循 `AnyObject`。与由语言本身定义的 `Any` 不同，`AnyObject` 由 Swift 标准库定义。更多信息请参阅 [仅类协议](protocols.md#Class-Only-Protocols) 和 [`AnyObject`](https://developer.apple.com/documentation/swift/anyobject)。

> Any 类型语法：
>
> *any-type* → **`Any`**

## Self 类型 {#Self-Type}

`Self` 类型不是某个特定类型，而是让你可以方便地引用当前类型，而不必重复或知道该类型的名称。

在协议声明或协议成员声明中，`Self` 类型指代最终遵循该协议的类型。

在结构体、类或枚举声明中，`Self` 类型指代该声明所引入的类型。在某个类型的成员的声明内部，`Self` 类型指代该类型。在类声明的成员中，`Self` 只能出现在以下位置：

- 作为方法的返回类型
- 作为只读下标的返回类型
- 作为只读计算属性的类型
- 在方法体中

例如，下面的代码展示了一个返回类型为 `Self` 的实例方法 `f`。

```swift
class Superclass {
    func f() -> Self { return self }
}
let x = Superclass()
print(type(of: x.f()))
// Prints "Superclass".

class Subclass: Superclass { }
let y = Subclass()
print(type(of: y.f()))
// Prints "Subclass".

let z: Superclass = Subclass()
print(type(of: z.f()))
// Prints "Subclass".
```

上面例子的最后一部分表明，`Self` 指代的是 `z` 的值的运行期类型 `Subclass`，而不是变量本身的编译期类型 `Superclass`。

在嵌套类型声明内部，`Self` 类型指代最内层类型声明所引入的类型。

`Self` 类型与 Swift 标准库中的 [`type(of:)`](https://developer.apple.com/documentation/swift/type(of:)) 函数指代相同的类型。写 `Self.someStaticMember` 来访问当前类型的成员，与写 `type(of: self).someStaticMember` 相同。

> Self 类型语法：
>
> *self-type* → **`Self`**

## 类型继承子句 {#Type-Inheritance-Clause}

*类型继承子句*（type inheritance clause）用于指定某个具名类型继承自哪个类，以及遵循哪些协议。类型继承子句以冒号（`:`）开头，后跟一个类型标识符列表。

类类型可以继承自单个父类，并遵循任意数量的协议。定义类时，父类的名称必须出现在类型标识符列表的第一个位置，后面是任意数量的该类必须遵循的协议。如果该类不继承自其他类，列表也可以改为以协议开头。关于类继承的详细讨论和若干示例，请参阅 [继承](inheritance.md)。

其他具名类型只能继承或遵循一个协议列表。协议类型可以继承任意数量的其他协议。当协议类型继承自其他协议时，那些其他协议的要求集合会被汇总在一起，任何继承当前协议的类型都必须遵循所有这些要求。在此列表中的协议名称前写一个波浪号（`~`）可以抑制对该协议的隐式遵循，详见 [协议的隐式一致性](protocols.md#Implicit-Conformance-to-a-Protocol)。

枚举定义中的类型继承子句可以是一个协议列表；或者，对于给成员分配原始值的枚举，也可以是指定这些原始值类型的单个具名类型。关于使用类型继承子句指定原始值类型的枚举定义示例，请参阅 [原始值](enumerations.md#Raw-Values)。

> 类型继承子句语法：
>
> *type-inheritance-clause* → **`:`** *type-inheritance-list* \
> *type-inheritance-list* → *attributes*_?_ **`~`**_?_ *type-identifier* | *attributes*_?_ **`~`**_?_ *type-identifier* **`,`** *type-inheritance-list*

## 类型推断 {#Type-Inference}

Swift 广泛使用*类型推断*（type inference），让你可以省略代码中许多变量和表达式的类型或类型的一部分。例如，你可以写 `var x = 0` 而不写 `var x: Int = 0`，完全省略类型——编译器会正确推断出 `x` 表示 `Int` 类型的值。同样，当完整类型可以从上下文推断出来时，你可以省略类型的一部分。例如，如果你写 `let dict: Dictionary = ["A": 1]`，编译器会推断出 `dict` 的类型是 `Dictionary<String, Int>`。

在上面的两个例子中，类型信息都是从表达式树的叶子向上传递到根节点。也就是说，`var x: Int = 0` 中 `x` 的类型是这样推断的：先检查 `0` 的类型，然后把这一类型信息向上传递到根节点（变量 `x`）。

在 Swift 中，类型信息也可以朝相反的方向流动——从根节点向下传递到叶子。例如，在下面的例子中，常量 `eFloat` 上的显式类型标注（`: Float`）使数值字面量 `2.71828` 的推断类型为 `Float` 而不是 `Double`。

```swift
let e = 2.71828 // The type of e is inferred to be Double.
let eFloat: Float = 2.71828 // The type of eFloat is Float.
```

Swift 中的类型推断在单个表达式或语句的层面上运作。这意味着，推断表达式中省略的类型或类型的一部分所需的所有信息，都必须能够从对该表达式或其某个子表达式进行类型检查中获得。
