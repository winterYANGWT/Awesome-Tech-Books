# 模式 {#Patterns}

匹配并解构值。

*模式*（pattern）表示单个值或复合值的结构。例如，元组 `(1, 2)` 的结构是由两个元素组成的逗号分隔列表。由于模式表示的是值的结构，而不是某个特定的值，因此可以用各种值来匹配它们。例如，模式 `(x, y)` 既匹配元组 `(1, 2)`，也匹配任何其他二元素元组。除了用模式匹配值，你还可以提取复合值的部分或全部内容，并把每一部分绑定到常量或变量名称。

在 Swift 中，模式分为两种基本类型：一种能成功匹配任何种类的值，另一种在运行时可能无法匹配指定的值。

第一种模式用于在简单的变量绑定、常量绑定和可选绑定中解构值。这类模式包括通配符模式、标识符模式，以及任何包含它们的值绑定模式或元组模式。你可以为这些模式指定类型标注，把它们限制为只匹配某种类型的值。

第二种模式用于完整的模式匹配，此时你尝试匹配的值在运行时可能并不存在。这类模式包括枚举成员模式、可选模式、表达式模式和类型转换模式。你可以在 `switch` 语句的 `case` 标签中、`do` 语句的 `catch` 子句中使用这些模式，也可以在 `if`、`while`、`guard` 或 `for`-`in` 语句的 `case` 条件中使用。

> 模式语法：
>
> *pattern* → *wildcard-pattern* *type-annotation*_?_ \
> *pattern* → *identifier-pattern* *type-annotation*_?_ \
> *pattern* → *value-binding-pattern* \
> *pattern* → *tuple-pattern* *type-annotation*_?_ \
> *pattern* → *enum-case-pattern* \
> *pattern* → *optional-pattern* \
> *pattern* → *type-casting-pattern* \
> *pattern* → *expression-pattern*

## 通配符模式 {#Wildcard-Pattern}

*通配符模式*（wildcard pattern）匹配并忽略任何值，由下划线（`_`）组成。当你不关心要匹配的值时，可以使用通配符模式。例如，下面的代码遍历闭区间 `1...3`，并在循环的每次迭代中忽略该区间当前的值：

```swift
for _ in 1...3 {
    // Do something three times.
}
```

> 通配符模式语法：
>
> *wildcard-pattern* → **`_`**

## 标识符模式 {#Identifier-Pattern}

*标识符模式*（identifier pattern）匹配任何值，并把匹配到的值绑定到变量或常量名称。例如，在下面的常量声明中，`someValue` 是一个标识符模式，它匹配类型为 `Int` 的值 `42`：

```swift
let someValue = 42
```

匹配成功后，值 `42` 会被绑定（赋值）到常量名称 `someValue`。

当变量声明或常量声明左侧的模式是标识符模式时，该标识符模式隐式地是值绑定模式的子模式。

> 标识符模式语法：
>
> *identifier-pattern* → *identifier*

## 值绑定模式 {#Value-Binding-Pattern}

*值绑定模式*（value-binding pattern）把匹配到的值绑定到变量或常量名称。将匹配值绑定到常量名称的值绑定模式以 `let` 关键字开头；绑定到变量名称的值绑定模式则以 `var` 关键字开头。

值绑定模式中的标识符模式会把新的具名变量或常量绑定到与之匹配的值。例如，你可以分解元组的元素，并把每个元素的值绑定到对应的标识符模式。

```swift
let point = (3, 2)
switch point {
// Bind x and y to the elements of point.
case let (x, y):
    print("The point is at (\(x), \(y)).")
}
// Prints "The point is at (3, 2)."
```

在上面的示例中，`let` 会分配到元组模式 `(x, y)` 中的每个标识符模式。由于这种行为，`switch` 的 `case let (x, y):` 和 `case (let x, let y):` 会匹配相同的值。

> 值绑定模式语法：
>
> *value-binding-pattern* → **`var`** *pattern* | **`let`** *pattern*

## 元组模式 {#Tuple-Pattern}

*元组模式*（tuple pattern）是用圆括号括起来的、包含零个或多个模式的逗号分隔列表。元组模式匹配相应元组类型的值。

你可以使用类型标注来约束元组模式，使其只匹配特定种类的元组类型。例如，常量声明 `let (x, y): (Int, Int) = (1, 2)` 中的元组模式 `(x, y): (Int, Int)` 只匹配两个元素都是 `Int` 类型的元组类型。

当元组模式用作 `for`-`in` 语句中的模式，或用于变量声明或常量声明中时，它只能包含通配符模式、标识符模式、可选模式，或包含这些模式的其他元组模式。例如，下面的代码无效，因为元组模式 `(x, 0)` 中的元素 `0` 是表达式模式：

```swift
let points = [(0, 0), (1, 0), (1, 1), (2, 0), (2, 1)]
// This code isn't valid.
for (x, 0) in points {
    /* ... */
}
```

仅含一个元素的元组模式外面的圆括号没有作用。该模式匹配这个单一元素类型的值。例如，以下写法是等价的：

```swift
let a = 2        // a: Int = 2
let (a) = 2      // a: Int = 2
let (a): Int = 2 // a: Int = 2
```

> 元组模式语法：
>
> *tuple-pattern* → **`(`** *tuple-pattern-element-list*_?_ **`)`** \
> *tuple-pattern-element-list* → *tuple-pattern-element* | *tuple-pattern-element* **`,`** *tuple-pattern-element-list* \
> *tuple-pattern-element* → *pattern* | *identifier* **`:`** *pattern*

## 枚举成员模式 {#Enumeration-Case-Pattern}

*枚举成员模式*（enumeration case pattern）匹配现有枚举类型的某个成员。枚举成员模式出现在 `switch` 语句的 `case` 标签中，以及 `if`、`while`、`guard` 和 `for`-`in` 语句的 `case` 条件中。

如果要匹配的枚举成员带有任何关联值，相应的枚举成员模式必须指定一个元组模式，其中为每个关联值包含一个元素。有关使用 `switch` 语句匹配带关联值的枚举成员的示例，参见 [关联值](enumerations.md#Associated-Values)。

枚举成员模式也能匹配包裹在可选类型中的该成员的值。这种简化语法让你可以省略可选模式。注意，由于 `Optional` 是作为枚举实现的，`.none` 和 `.some` 可以和该枚举类型的成员出现在同一个 switch 中。

```swift
enum SomeEnum { case left, right }
let x: SomeEnum? = .left
switch x {
case .left:
    print("Turn left")
case .right:
    print("Turn right")
case nil:
    print("Keep going straight")
}
// Prints "Turn left".
```

> 枚举成员模式语法：
>
> *enum-case-pattern* → *type-identifier*_?_ **`.`** *enum-case-name* *tuple-pattern*_?_

## 可选模式 {#Optional-Pattern}

*可选模式*（optional pattern）匹配包裹在 `Optional<Wrapped>` 枚举的 `some(Wrapped)` 成员中的值。可选模式由一个标识符模式紧跟一个问号组成，出现的位置与枚举成员模式相同。

由于可选模式是 `Optional` 枚举成员模式的语法糖，以下写法是等价的：

```swift
let someOptional: Int? = 42
// Match using an enumeration case pattern.
if case .some(let x) = someOptional {
    print(x)
}

// Match using an optional pattern.
if case let x? = someOptional {
    print(x)
}
```

可选模式提供了一种简便方式，用于在 `for`-`in` 语句中遍历可选值数组，只对非 `nil` 元素执行循环体。

```swift
let arrayOfOptionalInts: [Int?] = [nil, 2, 3, nil, 5]
// Match only non-nil values.
for case let number? in arrayOfOptionalInts {
    print("Found a \(number)")
}
// Found a 2
// Found a 3
// Found a 5
```

> 可选模式语法：
>
> *optional-pattern* → *identifier-pattern* **`?`**

## 类型转换模式 {#Type-Casting-Patterns}

类型转换模式有两种：`is` 模式和 `as` 模式。`is` 模式只出现在 `switch` 语句的 `case` 标签中。`is` 和 `as` 模式的形式如下：

```swift
is <#type#>
<#pattern#> as <#type#>
```

当值在运行时的类型与 `is` 模式右侧指定的类型相同，或者是该类型的子类时，`is` 模式就会匹配该值。`is` 模式的行为类似于 `is` 运算符，两者都执行类型转换，但会丢弃转换后返回的类型。

当值在运行时的类型与 `as` 模式右侧指定的类型相同，或者是该类型的子类时，`as` 模式就会匹配该值。如果匹配成功，匹配到的值的类型会被转换为 `as` 模式右侧指定的*模式*。

有关使用 `switch` 语句以 `is` 和 `as` 模式匹配值的示例，参见 [Any 和 AnyObject 的类型转换](type-casting.md#Type-Casting-for-Any-and-AnyObject)。

> 类型转换模式语法：
>
> *type-casting-pattern* → *is-pattern* | *as-pattern* \
> *is-pattern* → **`is`** *type* \
> *as-pattern* → *pattern* **`as`** *type*

## 表达式模式 {#Expression-Pattern}

*表达式模式*（expression pattern）表示一个表达式的值。表达式模式只出现在 `switch` 语句的 `case` 标签中。

表达式模式所表示的表达式，会使用 Swift 标准库中的模式匹配运算符（`~=`）与输入表达式的值进行比较。如果 `~=` 运算符返回 `true`，则匹配成功。默认情况下，`~=` 运算符使用 `==` 运算符比较两个相同类型的值。它还可以用值的区间来匹配值，即检查该值是否包含在这个区间内，如下例所示。

```swift
let point = (1, 2)
switch point {
case (0, 0):
    print("(0, 0) is at the origin.")
case (-2...2, -2...2):
    print("(\(point.0), \(point.1)) is near the origin.")
default:
    print("The point is at (\(point.0), \(point.1)).")
}
// Prints "(1, 2) is near the origin."
```

你可以重载 `~=` 运算符，提供自定义的表达式匹配行为。例如，你可以重写上面的示例，把 `point` 表达式与点的字符串表示进行比较。

```swift
// Overload the ~= operator to match a string with an integer.
func ~= (pattern: String, value: Int) -> Bool {
    return pattern == "\(value)"
}
switch point {
case ("0", "0"):
    print("(0, 0) is at the origin.")
default:
    print("The point is at (\(point.0), \(point.1)).")
}
// Prints "The point is at (1, 2)."
```

> 表达式模式语法：
>
> *expression-pattern* → *expression*
