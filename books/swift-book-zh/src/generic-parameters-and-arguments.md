# 泛型形参与实参 {#Generic-Parameters-and-Arguments}

将声明泛化，从而抽象掉具体类型。

本章介绍泛型类型、泛型函数和泛型构造器的形参与实参。当你声明泛型类型、泛型函数、泛型下标或泛型构造器时，需要指定该泛型类型、泛型函数或泛型构造器可以使用的类型形参。这些类型形参充当占位符，在创建泛型类型的实例，或者调用泛型函数或泛型构造器时，会被实际的具体类型实参替换。

关于 Swift 中泛型的概览，请参阅 [泛型](generics.md)。

## 泛型形参子句 {#Generic-Parameter-Clause}

*泛型形参子句*（generic parameter clause）指定泛型类型或泛型函数的类型形参，以及对这些形参的任何相关约束和要求。泛型形参子句用尖括号（<>）括起来，形式如下：

```swift
<<#generic parameter list#>>
```

*泛型形参列表*（generic parameter list）是由泛型形参组成的逗号分隔列表，每个泛型形参的形式如下：

```swift
<#type parameter#>: <#constraint#>
```

一个泛型形参由一个*类型形参*（type parameter）后跟一个可选的*约束*（constraint）组成。*类型形参*就是一个占位类型的名称（例如 `T`、`U`、`V`、`Key`、`Value` 等）。在类型、函数或构造器声明的其余部分，包括函数或构造器的签名中，你都可以使用这些类型形参（以及它们的任何关联类型）。

*约束*指定类型形参继承自某个特定的类，或者遵循某个协议或协议组合。例如，在下面的泛型函数中，泛型形参 `T: Comparable` 表示任何替换类型形参 `T` 的类型实参都必须遵循 `Comparable` 协议。

```swift
func simpleMax<T: Comparable>(_ x: T, _ y: T) -> T {
    if x < y {
        return y
    }
    return x
}
```

例如，由于 `Int` 和 `Double` 都遵循 `Comparable` 协议，这个函数可以接受这两种类型的实参。与泛型类型不同，使用泛型函数或泛型构造器时不需要指定泛型实参子句。类型实参改为根据传给函数或构造器的实参类型来推断。

```swift
simpleMax(17, 42) // T is inferred to be Int
simpleMax(3.14159, 2.71828) // T is inferred to be Double
```

泛型形参列表中的最后一个泛型形参后面可以跟一个可选的逗号。

### 整数泛型形参 {#Integer-Generic-Parameters}

*整数泛型形参*（integer generic parameter）是一个整数值的占位符，而不是类型的占位符。其形式如下：

```swift
let <#type parameter#>: <#type>
```

这里的*类型*必须是 Swift 标准库中的 `Int` 类型，或者是解析为 `Int` 的类型别名或泛型类型。

为整数泛型形参提供的值必须是一个整数字面量，或者来自外层泛型上下文的另一个整数泛型形参。例如：

```swift
struct SomeStruct<let x: Int> { }
let a: SomeStruct<2>  // OK: integer literal

struct AnotherStruct<let x: Int, T, each U> {
    let b: SomeStruct<x>  // OK: another integer generic parameter

    static let c = 42
    let d: SomeStruct<c>  // Error: Can't use a constant.

    let e: SomeStruct<T>  // Error: Can't use a generic type parameter.
    let f: SomeStruct<U>  // Error: Can't use a parameter pack.
}
```

类型上的整数泛型形参的值，可以作为该类型的静态常量成员访问，其可见性与类型本身相同。函数上的整数泛型形参的值，可以在函数内部作为常量访问。在表达式中使用时，这些常量的类型为 `Int`。

```swift
print(a.x)  // Prints "4"
```

整数泛型形参的值，可以从初始化该类型或调用该函数时所用实参的类型推断出来。

```swift
struct AnotherStruct<let y: Int> {
    var s: SomeStruct<y>
}
func someFunction<let z: Int>(s: SomeStruct<z>) {
    print(z)
}

let s1 = SomeStruct<12>()
let s2 = AnotherStruct(s: s1)  // AnotherStruct.y is inferred to be 12.
someFunction(s: s1)  // Prints "12"
```

### 泛型 where 子句 {#Generic-Where-Clauses}

你可以在类型或函数体左花括号的正前方加入泛型 `where` 子句，为类型形参及其关联类型指定额外的要求。泛型 `where` 子句由 `where` 关键字以及一个或多个*要求*（requirement）组成的逗号分隔列表构成。

```swift
where <#requirements#>
```

泛型 `where` 子句中的*要求*指定类型形参继承自某个类，或者遵循某个协议或协议组合。尽管泛型 `where` 子句为表达类型形参上的简单约束提供了语法糖（例如 `<T: Comparable>` 等价于 `<T> where T: Comparable` 等等），但你也可以用它为类型形参及其关联类型提供更复杂的约束。例如，你可以约束类型形参的关联类型遵循某些协议。例如，`<S: Sequence> where S.Iterator.Element: Equatable` 指定 `S` 遵循 `Sequence` 协议，并且关联类型 `S.Iterator.Element` 遵循 `Equatable` 协议。这个约束确保序列中的每个元素都可以比较相等。整数泛型形参不能有协议要求或父类要求。

你还可以使用 `==` 运算符指定两个类型相同的要求。例如，`<S1: Sequence, S2: Sequence> where S1.Iterator.Element == S2.Iterator.Element` 表达的约束是：`S1` 和 `S2` 都遵循 `Sequence` 协议，并且两个序列的元素必须是同一类型。对于整数泛型形参，`==` 运算符指定的是对其值的要求。你可以要求两个整数泛型形参具有相同的值，也可以为整数泛型形参要求一个特定的整数值。

任何替换类型形参的类型实参都必须满足施加在该类型形参上的所有约束和要求。

泛型 `where` 子句可以作为包含类型形参的声明的一部分出现，也可以作为嵌套在包含类型形参的声明内部的声明的一部分出现。嵌套声明的泛型 `where` 子句仍然可以引用外层声明的类型形参；不过，该 `where` 子句中的要求只适用于它所在的声明。

如果外层声明也有 `where` 子句，那么两个子句中的要求会合并。在下面的示例中，只有当 `Element` 同时遵循 `SomeProtocol` 和 `Numeric` 时，`startsWithZero()` 才可用。

```swift
extension Collection where Element: SomeProtocol {
    func startsWithZero() -> Bool where Element: Numeric {
        return first == .zero
    }
}
```

你可以通过在类型形参上提供不同的约束、要求或两者兼有，来重载泛型函数或泛型构造器。调用重载的泛型函数或泛型构造器时，编译器会使用这些约束来确定要调用哪个重载版本。

关于泛型 `where` 子句的更多信息，以及在泛型函数声明中使用它的示例，请参阅 [泛型 where 子句](generics.md#Generic-Where-Clauses)。

> 泛型形参子句语法：
>
> *generic-parameter-clause* → **`<`** *generic-parameter-list* **`,`**_?_ **`>`** \
> *generic-parameter-list* → *generic-parameter* | *generic-parameter* **`,`** *generic-parameter-list* \
> *generic-parameter* → *type-name* \
> *generic-parameter* → *type-name* **`:`** *type-identifier* \
> *generic-parameter* → *type-name* **`:`** *protocol-composition-type* \
> *generic-parameter* → **`let`** *type-name* **`:`** *type* \
>
> *generic-where-clause* → **`where`** *requirement-list* \
> *requirement-list* → *requirement* | *requirement* **`,`** *requirement-list* \
> *requirement* → *conformance-requirement* | *same-type-requirement*
>
> *conformance-requirement* → *type-identifier* **`:`** *type-identifier* \
> *conformance-requirement* → *type-identifier* **`:`** *protocol-composition-type* \
> *same-type-requirement* → *type-identifier* **`==`** *type* \
> *same-type-requirement* → *type-identifier* **`==`** *signed-integer-literal*

## 泛型实参子句 {#Generic-Argument-Clause}

*泛型实参子句*（generic argument clause）指定泛型类型的类型实参。泛型实参子句用尖括号（<>）括起来，形式如下：

```swift
<<#generic argument list#>>
```

*泛型实参列表*（generic argument list）是由类型实参组成的逗号分隔列表。*类型实参*（type argument）是实际具体类型的名称，用来替换泛型类型的泛型形参子句中对应的类型形参——对于整数泛型形参，则是替换该整数泛型形参的整数值。替换的结果是该泛型类型的一个特化版本。下面的示例展示了 Swift 标准库中泛型字典类型的一个简化版本。

```swift
struct Dictionary<Key: Hashable, Value>: Collection, ExpressibleByDictionaryLiteral {
    /* ... */
}
```

泛型 `Dictionary` 类型的特化版本 `Dictionary<String, Int>`，是通过把泛型形参 `Key: Hashable` 和 `Value` 替换为具体类型实参 `String` 和 `Int` 形成的。每个类型实参都必须满足它所替换的泛型形参的所有约束，包括泛型 `where` 子句中指定的任何额外要求。在上面的示例中，`Key` 类型形参被约束为遵循 `Hashable` 协议，因此 `String` 也必须遵循 `Hashable` 协议。

你也可以用一个本身就是某个泛型类型特化版本的类型实参来替换类型形参（前提是它满足相应的约束和要求）。例如，你可以把 `Array<Element>` 中的类型形参 `Element` 替换为数组的一个特化版本 `Array<Int>`，从而形成一个元素本身是整数数组的数组。

```swift
let arrayOfArrays: Array<Array<Int>> = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
```

泛型实参列表中的最后一个类型实参后面可以跟一个可选的逗号。

如 [泛型形参子句](generic-parameters-and-arguments.md#Generic-Parameter-Clause) 所述，泛型函数或泛型构造器的类型实参不用泛型实参子句来指定。

> 泛型实参子句语法：
>
> *generic-argument-clause* → **`<`** *generic-argument-list* **`,`**_?_ **`>`** \
> *generic-argument-list* → *generic-argument* | *generic-argument* **`,`** *generic-argument-list* \
> *generic-argument* → *type* | *signed-integer-literal*
