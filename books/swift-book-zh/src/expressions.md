# 表达式 {#Expressions}

访问、修改和赋值。

在 Swift 中，表达式分为四类：前缀表达式、中缀表达式、基本表达式和后缀表达式。对一个表达式求值会返回一个值、产生副作用，或两者兼有。

前缀表达式和中缀表达式让你把运算符作用于更小的表达式。基本表达式在概念上是最简单的一类表达式，它们提供了一种访问值的方式。后缀表达式与前缀表达式和中缀表达式一样，让你使用函数调用、成员访问等后缀来构建更复杂的表达式。下面各节会详细介绍每一类表达式。

> 表达式语法：
>
> *expression* → *try-operator*_?_ *await-operator*_?_ *prefix-expression* *infix-expressions*_?_

## 前缀表达式 {#Prefix-Expressions}

*前缀表达式*（prefix expression）将一个可选的前缀运算符与一个表达式组合在一起。前缀运算符接受一个操作数，即紧随其后的表达式。

关于这些运算符的行为，请参阅 [基本运算符](basic-operators.md) 和 [高级运算符](advanced-operators.md)。

关于 Swift 标准库提供的运算符，请参阅 [运算符声明](https://developer.apple.com/documentation/swift/operator_declarations)。

> 前缀表达式语法：
>
> *prefix-expression* → *prefix-operator*_?_ *postfix-expression* \
> *prefix-expression* → *in-out-expression*

### 输入输出表达式 {#In-Out-Expression}

*输入输出表达式*（in-out expression）标记一个变量，该变量正作为输入输出实参传给某个函数调用表达式。

```swift
&<#expression#>
```

关于输入输出形参的更多信息及示例，请参阅 [输入输出参数](functions.md#In-Out-Parameters)。

在需要指针的上下文中提供非指针实参时，也会用到输入输出表达式，详见 [隐式转换为指针类型](expressions.md#Implicit-Conversion-to-a-Pointer-Type)。

> 输入输出表达式语法：
>
> *in-out-expression* → **`&`** *primary-expression*

### try 运算符 {#Try-Operator}

*try 表达式*由 `try` 运算符后跟一个可能抛出错误的表达式构成。它的形式如下：

```swift
try <#expression#>
```

`try` 表达式的值就是该*表达式*的值。

*可选 try 表达式*由 `try?` 运算符后跟一个可能抛出错误的表达式构成。它的形式如下：

```swift
try? <#expression#>
```

如果该*表达式*没有抛出错误，可选 try 表达式的值就是一个包含该*表达式*值的可选值。否则，可选 try 表达式的值为 `nil`。

*强制 try 表达式*由 `try!` 运算符后跟一个可能抛出错误的表达式构成。它的形式如下：

```swift
try! <#expression#>
```

强制 try 表达式的值就是该*表达式*的值。如果该*表达式*抛出错误，就会产生运行时错误。

当中缀运算符左侧的表达式带有 `try`、`try?` 或 `try!` 标记时，该运算符会作用于整个中缀表达式。话虽如此，你也可以用圆括号明确指定运算符的作用范围。

```swift
// Writing 'try' applies to both function calls.
sum = try someThrowingFunction() + anotherThrowingFunction()

// Writing 'try' applies to both function calls.
sum = try (someThrowingFunction() + anotherThrowingFunction())

// Error: Writing 'try' applies only to the first function call.
sum = (try someThrowingFunction()) + anotherThrowingFunction()
```

`try` 表达式不能出现在中缀运算符的右侧，除非该中缀运算符是赋值运算符，或者该 `try` 表达式被圆括号括起来。

如果一个表达式同时包含 `try` 和 `await` 运算符，则 `try` 运算符必须出现在前面。

关于如何使用 `try`、`try?` 和 `try!` 的更多信息及示例，请参阅 [错误处理](error-handling.md)。

> try 表达式语法：
>
> *try-operator* → **`try`** | **`try`** **`?`** | **`try`** **`!`**

### await 运算符 {#Await-Operator}

*await 表达式*由 `await` 运算符后跟一个使用异步操作结果的表达式构成。它的形式如下：

```swift
await <#expression#>
```

`await` 表达式的值就是该*表达式*的值。

带有 `await` 标记的表达式称为*可能的挂起点*（potential suspension point）。异步函数的执行可以在每个带有 `await` 标记的表达式处被挂起。此外，并发代码的执行绝不会在任何其他位置被挂起。这意味着，可能的挂起点之间的代码可以安全地更新那些需要暂时打破不变式的状态，前提是它能在下一个可能的挂起点之前完成更新。

`await` 表达式只能出现在异步上下文中，例如传给 `async(priority:operation:)` 函数的尾随闭包。它不能出现在 `defer` 语句的语句体中，也不能出现在同步函数类型的自动闭包中。

当中缀运算符左侧的表达式带有 `await` 运算符标记时，该运算符会作用于整个中缀表达式。话虽如此，你也可以用圆括号明确指定运算符的作用范围。

```swift
// Writing 'await' applies to both function calls.
sum = await someAsyncFunction() + anotherAsyncFunction()

// Writing 'await' applies to both function calls.
sum = await (someAsyncFunction() + anotherAsyncFunction())

// Error: Writing 'await' applies only to the first function call.
sum = (await someAsyncFunction()) + anotherAsyncFunction()
```

`await` 表达式不能出现在中缀运算符的右侧，除非该中缀运算符是赋值运算符，或者该 `await` 表达式被圆括号括起来。

如果一个表达式同时包含 `await` 和 `try` 运算符，则 `try` 运算符必须出现在前面。

> await 表达式语法：
>
> *await-operator* → **`await`**

## 中缀表达式 {#Infix-Expressions}

*中缀表达式*（infix expression）将一个二元中缀运算符与作为其左右实参的表达式组合在一起。它的形式如下：

```swift
<#left-hand argument#> <#operator#> <#right-hand argument#>
```

关于这些运算符的行为，请参阅 [基本运算符](basic-operators.md) 和 [高级运算符](advanced-operators.md)。

关于 Swift 标准库提供的运算符，请参阅 [运算符声明](https://developer.apple.com/documentation/swift/operator_declarations)。

> 注意：在解析时，由中缀运算符组成的表达式会表示成一个扁平列表。通过应用运算符优先级，这个列表会被转换成树。例如，表达式 `2 + 3 * 5` 最初被理解为由五个项组成的扁平列表：`2`、`+`、`3`、`*` 和 `5`。这一过程会把它转换成树 (2 + (3 * 5))。

> 中缀表达式语法：
>
> *infix-expression* → *infix-operator* *prefix-expression* \
> *infix-expression* → *assignment-operator* *try-operator*_?_ *await-operator*_?_ *prefix-expression* \
> *infix-expression* → *conditional-operator* *try-operator*_?_ *await-operator*_?_ *prefix-expression* \
> *infix-expression* → *type-casting-operator* \
> *infix-expressions* → *infix-expression* *infix-expressions*_?_

### 赋值运算符 {#Assignment-Operator}

*赋值运算符*为给定的表达式设置一个新值。它的形式如下：

```swift
<#expression#> = <#value#>
```

把*表达式*的值设为对*值*求值所得到的值。如果*表达式*是元组，则*值*必须是具有相同元素个数的元组。（允许嵌套元组。）赋值会从*值*的每个部分分别进行到*表达式*的对应部分。例如：

```swift
(a, _, (b, c)) = ("test", 9.45, (12, 3))
// a is "test", b is 12, c is 3, and 9.45 is ignored
```

赋值运算符不返回任何值。

> 赋值运算符语法：
>
> *assignment-operator* → **`=`**

### 三元条件运算符 {#Ternary-Conditional-Operator}

*三元条件运算符*根据条件的值求值为两个给定值之一。它的形式如下：

```swift
<#condition#> ? <#expression used if true#> : <#expression used if false#>
```

如果*条件*求值为 `true`，条件运算符就求值第一个表达式并返回其值；否则，它求值第二个表达式并返回其值。未被使用的表达式不会被求值。

关于使用三元条件运算符的示例，请参阅 [三元条件运算符](basic-operators.md#Ternary-Conditional-Operator)。

> 条件运算符语法：
>
> *conditional-operator* → **`?`** *expression* **`:`**

### 类型转换运算符 {#Type-Casting-Operators}

类型转换运算符共有四种：`is` 运算符、`as` 运算符、`as?` 运算符和 `as!` 运算符。

它们的形式如下：

```swift
<#expression#> is <#type#>
<#expression#> as <#type#>
<#expression#> as? <#type#>
<#expression#> as! <#type#>
```

`is` 运算符在运行时检查该*表达式*能否转换为指定的*类型*。如果该*表达式*能转换为指定的*类型*，它就返回 `true`；否则返回 `false`。

当编译时已知转换一定会成功时（例如向上转型或桥接），`as` 运算符会执行转换。向上转型让你可以把一个表达式当作其类型的父类型的实例来使用，而不需要借助中间变量。下面几种写法是等价的：

```swift
func f(_ any: Any) { print("Function for Any") }
func f(_ int: Int) { print("Function for Int") }
let x = 10
f(x)
// Prints "Function for Int".

let y: Any = x
f(y)
// Prints "Function for Any".

f(x as Any)
// Prints "Function for Any".
```

桥接让你可以把 Swift 标准库类型（例如 `String`）的表达式当作其对应的 Foundation 类型（例如 `NSString`）来使用，而不需要创建新实例。关于桥接的更多信息，请参阅 [使用 Foundation 类型](https://developer.apple.com/documentation/swift/imported_c_and_objective_c_apis/working_with_foundation_types)。

`as?` 运算符对该*表达式*执行到指定*类型*的条件转换。`as?` 运算符返回指定*类型*的可选值。在运行时，如果转换成功，*表达式*的值会被包装在可选值中返回；否则返回的值为 `nil`。如果转换到指定*类型*保证会失败或保证会成功，就会引发编译时错误。

`as!` 运算符对该*表达式*执行到指定*类型*的强制转换。`as!` 运算符返回指定*类型*的值，而不是可选类型的值。如果转换失败，就会引发运行时错误。`x as! T` 的行为与 `(x as? T)!` 的行为相同。

关于类型转换的更多信息以及使用类型转换运算符的示例，请参阅 [类型转换](type-casting.md)。

> 类型转换运算符语法：
>
> *type-casting-operator* → **`is`** *type* \
> *type-casting-operator* → **`as`** *type* \
> *type-casting-operator* → **`as`** **`?`** *type* \
> *type-casting-operator* → **`as`** **`!`** *type*

## 基本表达式 {#Primary-Expressions}

*基本表达式*（primary expression）是最基本的一类表达式。它们可以单独作为表达式使用，也可以与其他词法单元组合，构成前缀表达式、中缀表达式和后缀表达式。

> 基本表达式语法：
>
> *primary-expression* → *identifier* *generic-argument-clause*_?_ \
> *primary-expression* → *literal-expression* \
> *primary-expression* → *self-expression* \
> *primary-expression* → *superclass-expression* \
> *primary-expression* → *conditional-expression* \
> *primary-expression* → *closure-expression* \
> *primary-expression* → *parenthesized-expression* \
> *primary-expression* → *tuple-expression* \
> *primary-expression* → *implicit-member-expression* \
> *primary-expression* → *wildcard-expression* \
> *primary-expression* → *macro-expansion-expression* \
> *primary-expression* → *key-path-expression* \
> *primary-expression* → *selector-expression* \
> *primary-expression* → *key-path-string-expression*

### 字面量表达式 {#Literal-Expression}

*字面量表达式*（literal expression）由普通字面量（例如字符串或数字）、数组字面量或字典字面量，或者 playground 字面量构成。

> 注意：在 Swift 5.9 之前，以下特殊字面量会被识别：`#column`、`#dsohandle`、`#fileID`、`#filePath`、`#file`、`#function` 和 `#line`。它们现在在 Swift 标准库中实现为宏：[`column()`](https://developer.apple.com/documentation/swift/column())、[`dsohandle()`](https://developer.apple.com/documentation/swift/dsohandle())、[`fileID()`](https://developer.apple.com/documentation/swift/fileID())、[`filePath()`](https://developer.apple.com/documentation/swift/filePath())、[`file()`](https://developer.apple.com/documentation/swift/file())、[`function()`](https://developer.apple.com/documentation/swift/function()) 和 [`line()`](https://developer.apple.com/documentation/swift/line())。

*数组字面量*是值的有序集合。它的形式如下：

```swift
[<#value 1#>, <#value 2#>, <#...#>]
```

数组中最后一个表达式后面可以跟一个可选的逗号。数组字面量的类型是 `[T]`，其中 `T` 是其中各表达式的类型。如果存在多种类型的表达式，`T` 是它们最接近的共同父类型。空数组字面量用一对空方括号表示，可用于创建指定类型的空数组。

```swift
var emptyArray: [Double] = []
```

*字典字面量*是键值对的无序集合。它的形式如下：

```swift
[<#key 1#>: <#value 1#>, <#key 2#>: <#value 2#>, <#...#>]
```

字典中最后一个表达式后面可以跟一个可选的逗号。字典字面量的类型是 `[Key: Value]`，其中 `Key` 是其键表达式的类型，`Value` 是其值表达式的类型。如果存在多种类型的表达式，`Key` 和 `Value` 分别是各自值最接近的共同父类型。空字典字面量写成一对方括号内加一个冒号（`[:]`），以便与空数组字面量区分。你可以用空字典字面量创建具有指定键类型和值类型的空字典。

```swift
var emptyDictionary: [String: Double] = [:]
```

*playground 字面量*由 Xcode 使用，用于在程序编辑器中创建颜色、文件或图像的可交互表示。在 Xcode 之外的纯文本中，playground 字面量使用一种特殊的字面量语法表示。

关于在 Xcode 中使用 playground 字面量的信息，请参阅 Xcode 帮助中的[添加颜色、文件或图像字面量](https://help.apple.com/xcode/mac/current/#/dev4c60242fc)。

> 字面量表达式语法：
>
> *literal-expression* → *literal* \
> *literal-expression* → *array-literal* | *dictionary-literal* | *playground-literal*
>
> *array-literal* → **`[`** *array-literal-items*_?_ **`,`**_?_ **`]`** \
> *array-literal-items* → *array-literal-item* | *array-literal-item* **`,`** *array-literal-items* \
> *array-literal-item* → *expression*
>
> *dictionary-literal* → **`[`** *dictionary-literal-items* **`,`**_?_ **`]`** | **`[`** **`:`** **`]`** \
> *dictionary-literal-items* → *dictionary-literal-item* | *dictionary-literal-item* **`,`** *dictionary-literal-items* \
> *dictionary-literal-item* → *expression* **`:`** *expression*
>
> *playground-literal* → **`#colorLiteral`** **`(`** **`red`** **`:`** *expression* **`,`** **`green`** **`:`** *expression* **`,`** **`blue`** **`:`** *expression* **`,`** **`alpha`** **`:`** *expression* **`)`** \
> *playground-literal* → **`#fileLiteral`** **`(`** **`resourceName`** **`:`** *expression* **`)`** \
> *playground-literal* → **`#imageLiteral`** **`(`** **`resourceName`** **`:`** *expression* **`)`**

### self 表达式 {#Self-Expression}

`self` 表达式显式引用当前类型，或引用它所在类型的当前实例。它的形式如下：

```swift
self
self.<#member name#>
self[<#subscript index#>]
self(<#initializer arguments#>)
self.init(<#initializer arguments#>)
```

在构造器、下标或实例方法中，`self` 指代它所在类型的当前实例。在类型方法中，`self` 指代它所在的当前类型。

访问成员时，`self` 表达式用于指定作用域；当作用域中存在另一个同名变量（例如函数形参）时，它可以消除歧义。例如：

```swift
class SomeClass {
    var greeting: String
    init(greeting: String) {
        self.greeting = greeting
    }
}
```

在值类型的可变方法中，你可以把该值类型的一个新实例赋给 `self`。例如：

```swift
struct Point {
    var x = 0.0, y = 0.0
    mutating func moveBy(x deltaX: Double, y deltaY: Double) {
        self = Point(x: x + deltaX, y: y + deltaY)
    }
}
```

> self 表达式语法：
>
> *self-expression* → **`self`** | *self-method-expression* | *self-subscript-expression* | *self-initializer-expression*
>
> *self-method-expression* → **`self`** **`.`** *identifier* \
> *self-subscript-expression* → **`self`** **`[`** *function-call-argument-list* **`]`** \
> *self-initializer-expression* → **`self`** **`.`** **`init`**

### 父类表达式 {#Superclass-Expression}

*父类表达式*（superclass expression）让类与其父类交互。它具有以下形式之一：

```swift
super.<#member name#>
super[<#subscript index#>]
super.init(<#initializer arguments#>)
```

第一种形式用于访问父类的成员。第二种形式用于访问父类的下标实现。第三种形式用于访问父类的构造器。

子类可以在实现成员、下标和构造器时使用父类表达式，以利用父类中的实现。

> 父类表达式语法：
>
> *superclass-expression* → *superclass-method-expression* | *superclass-subscript-expression* | *superclass-initializer-expression*
>
> *superclass-method-expression* → **`super`** **`.`** *identifier* \
> *superclass-subscript-expression* → **`super`** **`[`** *function-call-argument-list* **`]`** \
> *superclass-initializer-expression* → **`super`** **`.`** **`init`**

### 条件表达式 {#Conditional-Expression}

*条件表达式*（conditional expression）根据条件的值求值为若干个给定值之一。它具有以下形式之一：

```swift
if <#condition 1#> {
   <#expression used if condition 1 is true#>
} else if <#condition 2#> {
   <#expression used if condition 2 is true#>
} else {
   <#expression used if both conditions are false#>
}

switch <#expression#> {
case <#pattern 1#>:
    <#expression 1#>
case <#pattern 2#> where <#condition#>:
    <#expression 2#>
default:
    <#expression 3#>
}
```

条件表达式与 `if` 语句或 `switch` 语句的行为和语法相同，只是存在下面几段所描述的区别。

条件表达式只出现在以下上下文中：

  - 作为赋给变量的值。
  - 作为变量或常量声明中的初始值。
  - 作为 `throw` 表达式抛出的错误。
  - 作为函数、闭包或属性 getter 返回的值。
  - 作为条件表达式某个分支中的值。

条件表达式的各个分支是穷尽的，确保无论条件如何，该表达式总能产生一个值。这意味着每个 `if` 分支都需要有对应的 `else` 分支。

每个分支包含以下三者之一：单个表达式（当该分支的条件为真时，它被用作条件表达式的值）、`throw` 语句，或对永不返回的函数的调用。

每个分支都必须产生相同类型的值。由于每个分支的类型检查是独立的，有时你需要显式指定值的类型，例如当各分支包含不同类型的字面量，或某个分支的值为 `nil` 时。需要提供这类信息时，可以给接收结果的变量添加类型标注，或者给各分支的值添加 `as` 转换。

```swift
let number: Double = if someCondition { 10 } else { 12.34 }
let number = if someCondition { 10 as Double } else { 12.34 }
```

在结果构造器内部，条件表达式只能作为变量或常量的初始值出现。这意味着，当你在结果构造器中、变量或常量声明之外编写 `if` 或 `switch` 时，这段代码会被理解为分支语句，并由结果构造器的某个方法对其进行转换。

不要把条件表达式放进 `try` 表达式，即使条件表达式的某个分支会抛出错误。

> 条件表达式语法：
>
> *conditional-expression* → *if-expression* | *switch-expression*
>
> *if-expression* → **`if`** *condition-list* **`{`** *statement* **`}`** *if-expression-tail* \
> *if-expression-tail* → **`else`** *if-expression* \
> *if-expression-tail* → **`else`** **`{`** *statement* **`}`**
>
> *switch-expression* → **`switch`** *expression* **`{`** *switch-expression-cases* **`}`** \
> *switch-expression-cases* → *switch-expression-case* *switch-expression-cases*_?_ \
> *switch-expression-case* → *case-label* *statement* \
> *switch-expression-case* → *default-label* *statement*

### 闭包表达式 {#Closure-Expression}

*闭包表达式*（closure expression）创建一个闭包，在其他编程语言中，闭包也称为*lambda* 或*匿名函数*。与函数声明一样，闭包包含语句，并会从其外围作用域捕获常量和变量。它的形式如下：

```swift
{ (<#parameters#>) -> <#return type#> in
   <#statements#>
}
```

这些*形参*的形式与函数声明中的形参相同，详见 [函数声明](declarations.md#Function-Declaration)。

在闭包表达式中写 `throws` 或 `async`，会显式地把闭包标记为抛出或异步。

```swift
{ (<#parameters#>) async throws -> <#return type#> in
   <#statements#>
}
```

如果闭包的语句体包含 `throws` 语句，或包含一个未嵌套在具有穷尽错误处理的 `do` 语句中的 `try` 表达式，则该闭包会被理解为抛出闭包。如果抛出闭包只抛出单一类型的错误，该闭包就被理解为抛出该错误类型；否则，它会被理解为抛出 `any Error`。同样，如果语句体包含 `await` 表达式，该闭包会被理解为异步闭包。

有几种特殊形式可以让闭包写得更简洁：

- 闭包可以省略其形参的类型、返回类型，或两者都省略。如果同时省略形参名称和两种类型，还要省略语句前的 `in` 关键字。如果省略的类型无法被推断出来，就会引发编译时错误。
- 闭包可以省略形参名称。此时形参会隐式命名为 `$` 加上其位置：`$0`、`$1`、`$2`，依此类推。
- 只由一个表达式构成的闭包会被理解为返回该表达式的值。在对周围表达式进行类型推断时，也会考虑这个表达式的内容。

下面的闭包表达式是等价的：

```swift
myFunction { (x: Int, y: Int) -> Int in
    return x + y
}

myFunction { x, y in
    return x + y
}

myFunction { return $0 + $1 }

myFunction { $0 + $1 }
```

关于把闭包作为实参传给函数的信息，请参阅 [函数调用表达式](expressions.md#Function-Call-Expression)。

闭包表达式可以不存储在变量或常量中就直接使用，例如在函数调用中立即使用闭包。上面代码中传给 `myFunction` 的闭包表达式就是这种立即使用的例子。因此，闭包表达式是逃逸的还是非逃逸的，取决于该表达式周围的上下文。如果闭包表达式被立即调用，或作为非逃逸函数实参传入，它就是非逃逸的；否则，该闭包表达式就是逃逸的。

关于逃逸闭包的更多信息，请参阅 [逃逸闭包](closures.md#Escaping-Closures)。

#### 捕获列表 {#Capture-Lists}

默认情况下，闭包表达式会从外围作用域捕获常量和变量，并持有对这些值的强引用。你可以使用*捕获列表*（capture list）显式控制闭包中捕获值的方式。

捕获列表写在形参列表之前，是由方括号括起来的、以逗号分隔的表达式列表。如果使用捕获列表，就必须同时使用 `in` 关键字，即使省略了形参名称、形参类型和返回类型也是如此。捕获列表中最后一个表达式后面可以跟一个可选的逗号。

捕获列表中的条目在创建闭包时初始化。对于捕获列表中的每个条目，都会用一个常量初始化为外围作用域中同名常量或变量的值。例如，在下面的代码中，`a` 包含在捕获列表里，而 `b` 没有，因此它们的行为不同。

```swift
var a = 0
var b = 0
let closure = { [a] in
 print(a, b)
}

a = 10
b = 10
closure()
// Prints "0 10".
```

这里有两个不同的东西都叫 `a`：外围作用域中的变量和闭包作用域中的常量；但叫 `b` 的变量只有一个。创建闭包时，内层作用域中的 `a` 会用外层作用域中 `a` 的值初始化，但它们的值之间没有任何特殊关联。这意味着，改变外层作用域中 `a` 的值不会影响内层作用域中 `a` 的值，而在闭包内部改变 `a` 也不会影响闭包外部的 `a`。相比之下，叫 `b` 的变量只有一个——也就是外层作用域中的 `b`——因此无论在闭包内部还是外部修改它，两处都能看到变化。

当被捕获变量的类型具有引用语义时，这种区别就看不出来了。例如，下面的代码中有两个东西都叫 `x`：外层作用域中的变量和内层作用域中的常量；但由于引用语义，它们引用的是同一个对象。

```swift
class SimpleClass {
    var value: Int = 0
}
var x = SimpleClass()
var y = SimpleClass()
let closure = { [x] in
    print(x.value, y.value)
}

x.value = 10
y.value = 10
closure()
// Prints "10 10".
```

如果表达式值的类型是类，你可以在捕获列表中用 `weak` 或 `unowned` 标记该表达式，以捕获对该表达式值的弱引用或无主引用。

```swift
myFunction { print(self.title) }                    // implicit strong capture
myFunction { [self] in print(self.title) }          // explicit strong capture
myFunction { [weak self] in print(self!.title) }    // weak capture
myFunction { [unowned self] in print(self.title) }  // unowned capture
```

你也可以在捕获列表中把任意表达式绑定到一个具名值。该表达式在创建闭包时求值，捕获该值时会使用指定的引用强度。例如：

```swift
// Weak capture of "self.parent" as "parent".
myFunction { [weak parent = self.parent] in print(parent!.title) }
```

关于闭包表达式的更多信息和示例，请参阅 [闭包表达式](closures.md#Closure-Expressions)。关于捕获列表的更多信息和示例，请参阅 [解决闭包引起的强引用循环](automatic-reference-counting.md#Resolving-Strong-Reference-Cycles-for-Closures)。

> 闭包表达式语法：
>
> *closure-expression* → **`{`** *attributes*_?_ *closure-signature*_?_ *statements*_?_ **`}`**
>
> *closure-signature* → *capture-list*_?_ *closure-parameter-clause* **`async`**_?_ *throws-clause*_?_ *function-result*_?_ **`in`** \
> *closure-signature* → *capture-list* **`in`**
>
> *closure-parameter-clause* → **`(`** **`)`** | **`(`** *closure-parameter-list* **`,`**_?_ **`)`** | *identifier-list* \
> *closure-parameter-list* → *closure-parameter* | *closure-parameter* **`,`** *closure-parameter-list* \
> *closure-parameter* → *closure-parameter-name* *type-annotation*_?_ \
> *closure-parameter* → *closure-parameter-name* *type-annotation* **`...`** \
> *closure-parameter-name* → *identifier*
>
> *capture-list* → **`[`** *capture-list-items* **`,`**_?_ **`]`** \
> *capture-list-items* → *capture-list-item* | *capture-list-item* **`,`** *capture-list-items* \
> *capture-list-item* → *capture-specifier*_?_ *identifier* \
> *capture-list-item* → *capture-specifier*_?_ *identifier* **`=`** *expression* \
> *capture-list-item* → *capture-specifier*_?_ *self-expression* \
> *capture-specifier* → **`weak`** | **`unowned`** | **`unowned(safe)`** | **`unowned(unsafe)`**

### 隐式成员表达式 {#Implicit-Member-Expression}

*隐式成员表达式*（implicit member expression）是一种简写形式，用于在类型推断能够确定隐含类型的上下文中访问某个类型的成员，例如枚举成员或类型方法。它的形式如下：

```swift
.<#member name#>
```

例如：

```swift
var x = MyEnumeration.someValue
x = .anotherValue
```

如果推断出的类型是可选类型，你也可以在隐式成员表达式中使用非可选类型的成员。

```swift
var someOptional: MyEnumeration? = .someValue
```

隐式成员表达式后面可以跟一个后缀运算符，或 [后缀表达式](expressions.md#Postfix-Expressions) 中列出的其他后缀语法。这称为*链式隐式成员表达式*（chained implicit member expression）。虽然链式后缀表达式通常都具有相同的类型，但唯一的要求是：整个链式隐式成员表达式必须能转换为上下文所隐含的类型。具体来说，如果隐含类型是可选类型，你可以使用非可选类型的值；如果隐含类型是类类型，你可以使用其某个子类的值。例如：

```swift
class SomeClass {
    static var shared = SomeClass()
    static var sharedSubclass = SomeSubclass()
    var a = AnotherClass()
}
class SomeSubclass: SomeClass { }
class AnotherClass {
    static var s = SomeClass()
    func f() -> SomeClass { return AnotherClass.s }
}
let x: SomeClass = .shared.a.f()
let y: SomeClass? = .shared
let z: SomeClass = .sharedSubclass
```

在上面的代码中，`x` 的类型与上下文所隐含的类型完全一致，`y` 的类型可以从 `SomeClass` 转换为 `SomeClass?`，而 `z` 的类型可以从 `SomeSubclass` 转换为 `SomeClass`。

> 隐式成员表达式语法：
>
> *implicit-member-expression* → **`.`** *identifier* \
> *implicit-member-expression* → **`.`** *identifier* **`.`** *postfix-expression*

### 括号表达式 {#Parenthesized-Expression}

*括号表达式*（parenthesized expression）由一对圆括号括起来的表达式构成。你可以用圆括号显式地把表达式分组，从而指定运算的优先级。分组用的圆括号不会改变表达式的类型——例如，`(1)` 的类型就是 `Int`。

> 括号表达式语法：
>
> *parenthesized-expression* → **`(`** *expression* **`)`**

### 元组表达式 {#Tuple-Expression}

*元组表达式*（tuple expression）由一对圆括号括起来的、以逗号分隔的表达式列表构成。每个表达式前面可以有一个可选的标识符，两者用冒号（`:`）分隔。它的形式如下：

```swift
(<#identifier 1#>: <#expression 1#>, <#identifier 2#>: <#expression 2#>, <#...#>)
```

元组表达式中的每个标识符在该元组表达式的作用域内必须唯一。在嵌套的元组表达式中，同一嵌套层级的标识符必须唯一。例如，`(a: 10, a: 20)` 无效，因为标签 `a` 在同一层级出现了两次。然而，`(a: 10, b: (a: 1, x: 2))` 是有效的——虽然 `a` 出现了两次，但它在外层元组中出现一次，在内层元组中出现一次。

元组表达式可以不包含任何表达式，也可以包含两个或更多表达式。圆括号内只有一个表达式时，那是括号表达式。

> 注意：在 Swift 中，空元组表达式和空元组类型都写成 `()`。由于 `Void` 是 `()` 的类型别名，你可以用它来表示空元组类型。然而，和所有类型别名一样，`Void` 始终是一个类型——你不能用它来写空元组表达式。

元组中最后一个表达式后面可以跟一个可选的逗号。

> 元组表达式语法：
>
> *tuple-expression* → **`(`** **`)`** | **`(`** *tuple-element* **`,`** *tuple-element-list* **`,`**_?_ **`)`** \
> *tuple-element-list* → *tuple-element* | *tuple-element* **`,`** *tuple-element-list* \
> *tuple-element* → *expression* | *identifier* **`:`** *expression*

### 通配符表达式 {#Wildcard-Expression}

*通配符表达式*（wildcard expression）用于在赋值时显式忽略某个值。例如，在下面的赋值中，10 被赋给 `x`，而 20 被忽略：

```swift
(x, _) = (10, 20)
// x is 10, and 20 is ignored
```

> 通配符表达式语法：
>
> *wildcard-expression* → **`_`**

### 宏展开表达式 {#Macro-Expansion-Expression}

*宏展开表达式*（macro-expansion expression）由宏名称后跟一对圆括号中的、以逗号分隔的宏实参列表构成。宏在编译时展开。宏展开表达式具有以下形式：

```swift
<#macro name#>(<#macro argument 1#>, <#macro argument 2#>)
```

如果宏不接受任何实参，宏展开表达式会省略宏名称后的圆括号。

宏展开表达式可以作为形参的默认值出现。当用作函数或方法形参的默认值时，宏使用调用点的源代码位置求值，而不是使用它在函数定义中出现的位置。不过，当默认值是一个除宏之外还包含其他代码的较大表达式时，这些宏会在函数定义中出现的位置求值。

```swift
func f(a: Int = #line, b: Int = (#line), c: Int = 100 + #line) {
    print(a, b, c)
}
f()  // Prints "4 1 101"
```

在上面的函数中，`a` 的默认值是一个单独的宏表达式，因此该宏使用调用 `f(a:b:c:)` 处的源代码位置求值。相比之下，`b` 和 `c` 的值是包含宏的表达式——这些表达式中的宏使用定义 `f(a:b:c:)` 处的源代码位置求值。

把宏用作默认值时，编译器会在不展开宏的情况下对其进行类型检查，以检查以下要求：

- 宏的访问级别与使用它的函数相同，或限制更少。
- 宏要么不接受实参，要么其实参是不含字符串插值的字面量。
- 宏的返回类型与形参的类型匹配。

你用宏表达式调用独立宏。要调用附加宏，请使用 [特性](attributes.md) 中描述的自定义特性语法。独立宏和附加宏的展开方式都如下：

1. Swift 解析源代码，生成抽象语法树（AST）。

2. 宏实现接收 AST 节点作为输入，并执行该宏所需的转换。

3. 宏实现产生的、经过转换的 AST 节点会被添加到原始 AST 中。

每个宏的展开都是独立且自包含的。不过，作为性能优化，Swift 可能会启动一个实现该宏的外部进程，并复用同一个进程来展开多个宏。实现宏时，该代码不得依赖于你的代码之前展开过哪些宏，也不得依赖当前时间等任何其他外部状态。

对于嵌套宏以及具有多个角色的附加宏，展开过程会重复进行。嵌套的宏展开表达式由外向内展开。例如，在下面的代码中，`outerMacro(_:)` 先展开，而未展开的 `innerMacro(_:)` 调用会出现在 `outerMacro(_:)` 作为输入接收的抽象语法树中。

```swift
#outerMacro(12, #innerMacro(34), "some text")
```

具有多个角色的附加宏会为每个角色展开一次。每次展开都接收同一个原始 AST 作为输入。Swift 收集所有生成的 AST 节点，把它们放到 AST 中各自对应的位置，从而形成整体展开结果。

关于 Swift 中宏的概览，请参阅 [宏](macros.md)。

> 宏展开表达式语法：
>
> *macro-expansion-expression* → **`#`** *identifier* *generic-argument-clause*_?_ *function-call-argument-clause*_?_ *trailing-closures*_?_

### key path 表达式 {#Key-Path-Expression}

*key path 表达式*引用某个类型的属性或下标。你在键值观察等动态编程任务中使用 key path 表达式。它们的形式如下：

```swift
\<#type name#>.<#path#>
```

*类型名称*是具体类型的名称，包括所有泛型参数，例如 `String`、`[Int]` 或 `Set<Int>`。

*路径*由属性名、下标、可选链表达式和强制解包表达式构成。这些 key path 组件中的每一个都可以按任意顺序重复任意多次。

在编译时，key path 表达式会被替换为 [`KeyPath`](https://developer.apple.com/documentation/swift/keypath) 类的实例。

要使用 key path 访问值，请把该 key path 传给 `subscript(keyPath:)` 下标，所有类型都提供这个下标。例如：

```swift
struct SomeStructure {
    var someValue: Int
}

let s = SomeStructure(someValue: 12)
let pathToProperty = \SomeStructure.someValue

let value = s[keyPath: pathToProperty]
// value is 12
```

在类型推断能够确定隐含类型的上下文中，可以省略*类型名称*。下面的代码使用 `\.someProperty` 而不是 `\SomeClass.someProperty`：

```swift
class SomeClass: NSObject {
    @objc dynamic var someProperty: Int
    init(someProperty: Int) {
        self.someProperty = someProperty
    }
}

let c = SomeClass(someProperty: 10)
c.observe(\.someProperty) { object, change in
    // ...
}
```

*路径*可以引用 `self`，以创建恒等 key path（`\.self`）。恒等 key path 指代整个实例，因此你可以用它一步访问并修改存储在变量中的所有数据。例如：

```swift
var compoundValue = (a: 1, b: 2)
// Equivalent to compoundValue = (a: 10, b: 20)
compoundValue[keyPath: \.self] = (a: 10, b: 20)
```

*路径*可以包含多个用句点分隔的属性名，以引用属性值中的属性。下面的代码使用 key path 表达式 `\OuterStructure.outer.someValue` 访问 `OuterStructure` 类型的 `outer` 属性中的 `someValue` 属性：

```swift
struct OuterStructure {
    var outer: SomeStructure
    init(someValue: Int) {
        self.outer = SomeStructure(someValue: someValue)
    }
}

let nested = OuterStructure(someValue: 24)
let nestedKeyPath = \OuterStructure.outer.someValue

let nestedValue = nested[keyPath: nestedKeyPath]
// nestedValue is 24
```

*路径*可以包含用方括号表示的下标，只要下标的形参类型遵循 `Hashable` 协议。下面这个例子在 key path 中使用下标访问数组的第二个元素：

```swift
let greetings = ["hello", "hola", "bonjour", "안녕"]
let myGreeting = greetings[keyPath: \[String].[1]]
// myGreeting is 'hola'
```

下标中使用的值可以是具名值或字面量。key path 中的值按值语义捕获。下面的代码在 key path 表达式和闭包中都使用变量 `index`，以访问 `greetings` 数组的第三个元素。修改 `index` 后，key path 表达式仍然引用第三个元素，而闭包会使用新的索引。

```swift
var index = 2
let path = \[String].[index]
let fn: ([String]) -> String = { strings in strings[index] }

print(greetings[keyPath: path])
// Prints "bonjour".
print(fn(greetings))
// Prints "bonjour".

// Setting 'index' to a new value doesn't affect 'path'
index += 1
print(greetings[keyPath: path])
// Prints "bonjour".

// Because 'fn' closes over 'index', it uses the new value
print(fn(greetings))
// Prints "안녕".
```

*路径*可以使用可选链和强制解包。下面的代码在 key path 中使用可选链访问一个可选字符串的属性：

```swift
let firstGreeting: String? = greetings.first
print(firstGreeting?.count as Any)
// Prints "Optional(5)".

// Do the same thing using a key path.
let count = greetings[keyPath: \[String].first?.count]
print(count as Any)
// Prints "Optional(5)".
```

你可以混合搭配 key path 的各个组件，以访问类型中深层嵌套的值。下面的代码使用组合了这些组件的 key path 表达式，访问一个数组字典中的不同值和属性。

```swift
let interestingNumbers = ["prime": [2, 3, 5, 7, 11, 13, 17],
                          "triangular": [1, 3, 6, 10, 15, 21, 28],
                          "hexagonal": [1, 6, 15, 28, 45, 66, 91]]
print(interestingNumbers[keyPath: \[String: [Int]].["prime"]] as Any)
// Prints "Optional([2, 3, 5, 7, 11, 13, 17])".
print(interestingNumbers[keyPath: \[String: [Int]].["prime"]![0]])
// Prints "2".
print(interestingNumbers[keyPath: \[String: [Int]].["hexagonal"]!.count])
// Prints "7".
print(interestingNumbers[keyPath: \[String: [Int]].["hexagonal"]!.count.bitWidth])
// Prints "64".
```

在通常需要提供函数或闭包的上下文中，你可以使用 key path 表达式。具体来说，如果某个 key path 表达式的根类型是 `SomeType`，其路径产生的值的类型是 `Value`，那么你可以用它代替类型为 `(SomeType) -> Value` 的函数或闭包。

```swift
struct Task {
    var description: String
    var completed: Bool
}
var toDoList = [
    Task(description: "Practice ping-pong.", completed: false),
    Task(description: "Buy a pirate costume.", completed: true),
    Task(description: "Visit Boston in the Fall.", completed: false),
]

// Both approaches below are equivalent.
let descriptions = toDoList.filter(\.completed).map(\.description)
let descriptions2 = toDoList.filter { $0.completed }.map { $0.description }
```

key path 表达式的任何副作用都只在求值该表达式时发生。例如，如果你在 key path 表达式的下标中调用函数，该函数只会在求值该表达式时被调用一次，而不是每次使用 key path 时都调用。

```swift
func makeIndex() -> Int {
    print("Made an index")
    return 0
}
// The line below calls makeIndex().
let taskKeyPath = \[Task][makeIndex()]
// Prints "Made an index".

// Using taskKeyPath doesn't call makeIndex() again.
let someTask = toDoList[keyPath: taskKeyPath]
```

关于在与 Objective-C API 交互的代码中使用 key path 的更多信息，请参阅[在 Swift 中使用 Objective-C 运行时特性](https://developer.apple.com/documentation/swift/using_objective_c_runtime_features_in_swift)。关于键值编码和键值观察的信息，请参阅[键值编码编程指南](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/KeyValueCoding/index.html#//apple_ref/doc/uid/10000107i)和[键值观察编程指南](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/KeyValueObserving/KeyValueObserving.html#//apple_ref/doc/uid/10000177i)。

> key path 表达式语法：
>
> *key-path-expression* → **`\`** *type*_?_ **`.`** *key-path-components* \
> *key-path-components* → *key-path-component* | *key-path-component* **`.`** *key-path-components* \
> *key-path-component* → *identifier* *key-path-postfixes*_?_ | *key-path-postfixes*
>
> *key-path-postfixes* → *key-path-postfix* *key-path-postfixes*_?_ \
> *key-path-postfix* → **`?`** | **`!`** | **`self`** | **`[`** *function-call-argument-list* **`]`**

### 选择器表达式 {#Selector-Expression}

选择器表达式让你访问 Objective-C 中用来引用方法或属性 getter、setter 的选择器。它的形式如下：

```swift
#selector(<#method name#>)
#selector(getter: <#property name#>)
#selector(setter: <#property name#>)
```

*方法名称*和*属性名称*必须是对 Objective-C 运行时中可用的方法或属性的引用。选择器表达式的值是一个 `Selector` 类型的实例。例如：

```swift
class SomeClass: NSObject {
    @objc let property: String

    @objc(doSomethingWithInt:)
    func doSomething(_ x: Int) { }

    init(property: String) {
        self.property = property
    }
}
let selectorForMethod = #selector(SomeClass.doSomething(_:))
let selectorForPropertyGetter = #selector(getter: SomeClass.property)
```

为属性的 getter 创建选择器时，*属性名称*可以引用变量属性或常量属性。相比之下，为属性的 setter 创建选择器时，*属性名称*只能引用变量属性。

*方法名称*可以包含用于分组的圆括号，也可以包含 `as` 运算符，以区分名称相同但类型签名不同的方法。例如：

```swift
extension SomeClass {
    @objc(doSomethingWithString:)
    func doSomething(_ x: String) { }
}
let anotherSelector = #selector(SomeClass.doSomething(_:) as (SomeClass) -> (String) -> Void)
```

由于选择器在编译时而不是运行时创建，编译器可以检查方法或属性是否存在，以及它们是否暴露给 Objective-C 运行时。

> 注意：虽然*方法名称*和*属性名称*是表达式，但它们从不会被求值。

关于在与 Objective-C API 交互的 Swift 代码中使用选择器的更多信息，请参阅[在 Swift 中使用 Objective-C 运行时特性](https://developer.apple.com/documentation/swift/using_objective_c_runtime_features_in_swift)。

> 选择器表达式语法：
>
> *selector-expression* → **`#selector`** **`(`** *expression* **`)`** \
> *selector-expression* → **`#selector`** **`(`** **`getter:`** *expression* **`)`** \
> *selector-expression* → **`#selector`** **`(`** **`setter:`** *expression* **`)`**

### key path 字符串表达式 {#Key-Path-String-Expression}

key path 字符串表达式让你访问在 Objective-C 中用于引用属性的字符串，以便用于键值编码和键值观察 API。它的形式如下：

```swift
#keyPath(<#property name#>)
```

*属性名称*必须是对 Objective-C 运行时中可用的属性的引用。在编译时，key path 字符串表达式会被替换为字符串字面量。例如：

```swift
class SomeClass: NSObject {
    @objc var someProperty: Int
    init(someProperty: Int) {
       self.someProperty = someProperty
    }
}

let c = SomeClass(someProperty: 12)
let keyPath = #keyPath(SomeClass.someProperty)

if let value = c.value(forKey: keyPath) {
    print(value)
}
// Prints "12".
```

在类中使用 key path 字符串表达式时，你可以只写属性名称而不写类名，来引用该类的属性。

```swift
extension SomeClass {
    func getSomeKeyPath() -> String {
        return #keyPath(someProperty)
    }
}
print(keyPath == c.getSomeKeyPath())
// Prints "true".
```

由于 key path 字符串在编译时而不是运行时创建，编译器可以检查该属性是否存在，以及该属性是否暴露给 Objective-C 运行时。

关于在与 Objective-C API 交互的 Swift 代码中使用 key path 的更多信息，请参阅[在 Swift 中使用 Objective-C 运行时特性](https://developer.apple.com/documentation/swift/using_objective_c_runtime_features_in_swift)。关于键值编码和键值观察的信息，请参阅[键值编码编程指南](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/KeyValueCoding/index.html#//apple_ref/doc/uid/10000107i)和[键值观察编程指南](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/KeyValueObserving/KeyValueObserving.html#//apple_ref/doc/uid/10000177i)。

> 注意：虽然*属性名称*是表达式，但它从不会被求值。

> key path 字符串表达式语法：
>
> *key-path-string-expression* → **`#keyPath`** **`(`** *expression* **`)`**

## 后缀表达式 {#Postfix-Expressions}

*后缀表达式*（postfix expression）由对表达式应用后缀运算符或其他后缀语法构成。从语法上说，每个基本表达式也都是后缀表达式。

关于这些运算符的行为，请参阅 [基本运算符](basic-operators.md) 和 [高级运算符](advanced-operators.md)。

关于 Swift 标准库提供的运算符，请参阅 [运算符声明](https://developer.apple.com/documentation/swift/operator_declarations)。

> 后缀表达式语法：
>
> *postfix-expression* → *primary-expression* \
> *postfix-expression* → *postfix-expression* *postfix-operator* \
> *postfix-expression* → *function-call-expression* \
> *postfix-expression* → *initializer-expression* \
> *postfix-expression* → *explicit-member-expression* \
> *postfix-expression* → *postfix-self-expression* \
> *postfix-expression* → *subscript-expression* \
> *postfix-expression* → *forced-value-expression* \
> *postfix-expression* → *optional-chaining-expression*

### 函数调用表达式 {#Function-Call-Expression}

*函数调用表达式*（function call expression）由函数名称后跟一对圆括号中的、以逗号分隔的函数实参列表构成。函数调用表达式具有以下形式：

```swift
<#function name#>(<#argument value 1#>, <#argument value 2#>)
```

*函数名称*可以是任何值为函数类型的表达式。

如果函数定义中为形参提供了名称，函数调用就必须在实参值前面写上这些名称，两者用冒号（`:`）分隔。这种函数调用表达式的形式如下：

```swift
<#function name#>(<#argument name 1#>: <#argument value 1#>, <#argument name 2#>: <#argument value 2#>)
```

圆括号中的最后一个实参后面可以跟一个可选的逗号。

函数调用表达式可以在右圆括号之后紧接着包含闭包表达式形式的尾随闭包。尾随闭包会被理解为函数的实参，添加在最后一个括号内实参之后。第一个闭包表达式不带实参标签；任何额外的闭包表达式前面都要写上各自的实参标签。下面的示例展示了使用和不使用尾随闭包语法的等价函数调用：

```swift
// someFunction takes an integer and a closure as its arguments
someFunction(x: x, f: { $0 == 13 })
someFunction(x: x) { $0 == 13 }

// anotherFunction takes an integer and two closures as its arguments
anotherFunction(x: x, f: { $0 == 13 }, g: { print(99) })
anotherFunction(x: x) { $0 == 13 } g: { print(99) }
```

如果尾随闭包是函数的唯一实参，你可以省略圆括号。

```swift
// someMethod takes a closure as its only argument
myData.someMethod() { $0 == 13 }
myData.someMethod { $0 == 13 }
```

为了把尾随闭包纳入实参，编译器会按如下方式从左到右检查函数的形参：

| 尾随闭包 | 形参 | 操作 |
| ---------------- | --------- | ------ |
| 带标签 | 带标签 | 如果标签相同，闭包与该形参匹配；否则跳过该形参。 |
| 带标签 | 不带标签 | 跳过该形参。 |
| 不带标签 | 带标签或不带标签 | 如果该形参在结构上类似于函数类型（定义见下文），闭包就与该形参匹配；否则跳过该形参。 |

尾随闭包会作为它所匹配的形参的实参传入。扫描过程中被跳过的形参不会收到任何实参——例如，它们可以使用默认参数。找到匹配后，扫描会继续处理下一个尾随闭包和下一个形参。匹配过程结束时，所有尾随闭包都必须有匹配的形参。

如果形参不是输入输出形参，并且属于以下情况之一，则该形参*在结构上类似于*函数类型：

- 形参的类型是函数类型，例如 `(Bool) -> Int`。
- 形参是自动闭包，且其被包装表达式的类型是函数类型，例如 `@autoclosure () -> ((Bool) -> Int)`。
- 形参是可变参数，且其数组元素类型是函数类型，例如 `((Bool) -> Int)...`。
- 形参的类型被一层或多层可选类型包裹，例如 `Optional<(Bool) -> Int>`。
- 形参的类型组合了这些允许的类型，例如 `(Optional<(Bool) -> Int>)...`。

当尾随闭包匹配到某个形参，而该形参的类型在结构上类似于函数类型、但本身不是函数时，闭包会按需被包装。例如，如果形参的类型是可选类型，闭包会自动包装进 `Optional`。

为了便于从 Swift 5.3 之前的版本迁移代码（这些版本从右到左执行匹配），编译器会同时检查从左到右和从右到左两种顺序。如果两种扫描方向产生不同结果，就使用旧的从右到左顺序，并且编译器会生成警告。未来的 Swift 版本将始终使用从左到右的顺序。

```swift
typealias Callback = (Int) -> Int
func someFunction(firstClosure: Callback? = nil,
                secondClosure: Callback? = nil) {
    let first = firstClosure?(10)
    let second = secondClosure?(20)
    print(first ?? "-", second ?? "-")
}

someFunction()  // Prints "- -"
someFunction { return $0 + 100 }  // Ambiguous
someFunction { return $0 } secondClosure: { return $0 }  // Prints "10 20"
```

在上面的示例中，标记为“Ambiguous”的函数调用在 Swift 5.3 上会打印“- 120”并产生编译器警告。未来的 Swift 版本将打印“110 -”。

类、结构体或枚举类型可以通过声明若干方法之一，为函数调用语法提供语法糖，详见 [具有特殊名称的方法](declarations.md#Methods-with-Special-Names)。

#### 隐式转换为指针类型 {#Implicit-Conversion-to-a-Pointer-Type}

在函数调用表达式中，如果实参和形参的类型不同，编译器会尝试应用下面列表中的一种隐式转换，使它们的类型匹配：

- `inout SomeType` 可以变成 `UnsafePointer<SomeType>` 或 `UnsafeMutablePointer<SomeType>`
- `inout Array<SomeType>` 可以变成 `UnsafePointer<SomeType>` 或 `UnsafeMutablePointer<SomeType>`
- `Array<SomeType>` 可以变成 `UnsafePointer<SomeType>`
- `String` 可以变成 `UnsafePointer<CChar>`

下面两个函数调用是等价的：

```swift
func unsafeFunction(pointer: UnsafePointer<Int>) {
    // ...
}
var myNumber = 1234

unsafeFunction(pointer: &myNumber)
withUnsafePointer(to: myNumber) { unsafeFunction(pointer: $0) }
```

通过这些隐式转换创建的指针仅在函数调用期间有效。为了避免未定义行为，请确保你的代码绝不会在函数调用结束后继续保存该指针。

> 注意：把数组隐式转换为不安全指针时，Swift 会按需转换或复制数组，以确保数组的存储是连续的。例如，对于从 `NSArray` 子类桥接到 `Array` 的数组（该子类未对其存储作出任何 API 约定），你可以使用这种语法。如果你需要保证数组的存储已经是连续的，从而让隐式转换永远不需要做这项工作，请使用 `ContiguousArray` 而不是 `Array`。

使用 `&` 而不是 `withUnsafePointer(to:)` 这样的显式函数，可以让对底层 C 函数的调用更易读，尤其是当函数接受多个指针实参时。不过，在调用其他 Swift 代码中的函数时，应避免用 `&` 代替显式使用不安全 API。

> 函数调用表达式语法：
>
> *function-call-expression* → *postfix-expression* *function-call-argument-clause* \
> *function-call-expression* → *postfix-expression* *function-call-argument-clause*_?_ *trailing-closures*
>
> *function-call-argument-clause* → **`(`** **`)`** | **`(`** *function-call-argument-list* **`,`**_?_ **`)`** \
> *function-call-argument-list* → *function-call-argument* | *function-call-argument* **`,`** *function-call-argument-list* \
> *function-call-argument* → *expression* | *identifier* **`:`** *expression* \
> *function-call-argument* → *operator* | *identifier* **`:`** *operator*
>
> *trailing-closures* → *closure-expression* *labeled-trailing-closures*_?_ \
> *labeled-trailing-closures* → *labeled-trailing-closure* *labeled-trailing-closures*_?_ \
> *labeled-trailing-closure* → *identifier* **`:`** *closure-expression*

### 构造器表达式 {#Initializer-Expression}

*构造器表达式*（initializer expression）提供对某个类型构造器的访问。它的形式如下：

```swift
<#expression#>.init(<#initializer arguments#>)
```

你在函数调用表达式中使用构造器表达式来初始化某个类型的新实例。你也使用构造器表达式来委托给父类的构造器。

```swift
class SomeSubClass: SomeSuperClass {
    override init() {
        // subclass initialization goes here
        super.init()
    }
}
```

与函数一样，构造器也可以作为值使用。例如：

```swift
// Type annotation is required because String has multiple initializers.
let initializer: (Int) -> String = String.init
let oneTwoThree = [1, 2, 3].map(initializer).reduce("", +)
print(oneTwoThree)
// Prints "123".
```

如果按名称指定类型，你可以不使用构造器表达式就访问该类型的构造器。在其他所有情况下，都必须使用构造器表达式。

```swift
let s1 = SomeType.init(data: 3)  // Valid
let s2 = SomeType(data: 1)       // Also valid

let s3 = type(of: someValue).init(data: 7)  // Valid
let s4 = type(of: someValue)(data: 5)       // Error
```

> 构造器表达式语法：
>
> *initializer-expression* → *postfix-expression* **`.`** **`init`** \
> *initializer-expression* → *postfix-expression* **`.`** **`init`** **`(`** *argument-names* **`)`**

### 显式成员表达式 {#Explicit-Member-Expression}

*显式成员表达式*（explicit member expression）允许访问具名类型、元组或模块的成员。它由该项与成员标识符之间的句点（`.`）构成。

```swift
<#expression#>.<#member name#>
```

具名类型的成员在类型声明或扩展中命名。例如：

```swift
class SomeClass {
    var someProperty = 42
}
let c = SomeClass()
let y = c.someProperty  // Member access
```

元组的成员按出现顺序从零开始用整数隐式命名。例如：

```swift
var t = (10, 20, 30)
t.0 = t.1
// Now t is (20, 20, 30)
```

模块的成员用于访问该模块的顶层声明。

用 `dynamicMemberLookup` 特性声明的类型包含在运行时查找的成员，详见 [特性](attributes.md)。

要区分仅实参名称不同的方法或构造器，请在圆括号中写出实参名称，每个实参名称后跟一个冒号（`:`）。对于没有名称的实参，写一个下划线（`_`）。要区分重载方法，请使用类型标注。例如：

```swift
class SomeClass {
    func someMethod(x: Int, y: Int) {}
    func someMethod(x: Int, z: Int) {}
    func overloadedMethod(x: Int, y: Int) {}
    func overloadedMethod(x: Int, y: Bool) {}
}
let instance = SomeClass()

let a = instance.someMethod              // Ambiguous
let b = instance.someMethod(x:y:)        // Unambiguous

let d = instance.overloadedMethod        // Ambiguous
let d = instance.overloadedMethod(x:y:)  // Still ambiguous
let d: (Int, Bool) -> Void  = instance.overloadedMethod(x:y:)  // Unambiguous
```

如果句点出现在行首，它会被理解为显式成员表达式的一部分，而不是隐式成员表达式。例如，下面的代码展示了拆分成多行的链式方法调用：

```swift
let x = [10, 3, 20, 15, 4]
    .sorted()
    .filter { $0 > 5 }
    .map { $0 * 100 }
```

你可以把这种多行链式语法与编译器控制语句结合使用，以控制每个方法的调用时机。例如，下面的代码在 iOS 上使用不同的过滤规则：

```swift
let numbers = [10, 20, 33, 43, 50]
#if os(iOS)
    .filter { $0 < 40 }
#else
    .filter { $0 > 25 }
#endif
```

在 `#if`、`#endif` 和其他编译指令之间，条件编译块可以包含一个隐式成员表达式后跟零个或多个后缀，从而构成后缀表达式。它也可以包含另一个条件编译块，或这些表达式和块的组合。

你可以在任何能写显式成员表达式的地方使用这种语法，不仅限于顶层代码。

在条件编译块中，`#if` 编译指令对应的分支必须至少包含一个表达式。其他分支可以为空。

> 显式成员表达式语法：
>
> *explicit-member-expression* → *postfix-expression* **`.`** *decimal-digits* \
> *explicit-member-expression* → *postfix-expression* **`.`** *identifier* *generic-argument-clause*_?_ \
> *explicit-member-expression* → *postfix-expression* **`.`** *identifier* **`(`** *argument-names* **`)`** \
> *explicit-member-expression* → *postfix-expression* *conditional-compilation-block*
>
> *argument-names* → *argument-name* *argument-names*_?_ \
> *argument-name* → *identifier* **`:`**

### 后缀 self 表达式 {#Postfix-Self-Expression}

后缀 `self` 表达式由表达式或类型名称紧随 `.self` 构成。它的形式如下：

```swift
<#expression#>.self
<#type#>.self
```

第一种形式求值为该*表达式*的值。例如，`x.self` 求值为 `x`。

第二种形式求值为该*类型*的值。使用这种形式把类型作为值来访问。例如，由于 `SomeClass.self` 求值为 `SomeClass` 类型本身，你可以把它传给接受类型层面的实参的函数或方法。

> 后缀 self 表达式语法：
>
> *postfix-self-expression* → *postfix-expression* **`.`** **`self`**

### 下标表达式 {#Subscript-Expression}

*下标表达式*（subscript expression）使用对应下标声明的 getter 和 setter 提供下标访问。它的形式如下：

```swift
<#expression#>[<#index expressions#>]
```

要对下标表达式求值，会调用该*表达式*类型的下标 getter，并把*索引表达式*作为下标形参传入。要设置其值，则以相同方式调用下标 setter。

关于下标声明的信息，请参阅 [协议下标声明](declarations.md#Protocol-Subscript-Declaration)。

> 下标表达式语法：
>
> *subscript-expression* → *postfix-expression* **`[`** *function-call-argument-list* **`]`**

### 强制取值表达式 {#Forced-Value-Expression}

*强制取值表达式*（forced-value expression）会解包一个你确信不为 `nil` 的可选值。它的形式如下：

```swift
<#expression#>!
```

如果该*表达式*的值不为 `nil`，就解包该可选值，并以对应的非可选类型返回。否则，会引发运行时错误。

强制取值表达式解包后的值可以被修改，既可以修改值本身，也可以给该值的某个成员赋值。例如：

```swift
var x: Int? = 0
x! += 1
// x is now 1

var someDictionary = ["a": [1, 2, 3], "b": [10, 20]]
someDictionary["a"]![0] = 100
// someDictionary is now ["a": [100, 2, 3], "b": [10, 20]]
```

> 强制取值表达式语法：
>
> *forced-value-expression* → *postfix-expression* **`!`**

### 可选链表达式 {#Optional-Chaining-Expression}

*可选链表达式*（optional-chaining expression）提供了一种简化语法，用于在后缀表达式中使用可选值。它的形式如下：

```swift
<#expression#>?
```

后缀 `?` 运算符把一个表达式变成可选链表达式，同时不改变该表达式的值。

可选链表达式必须出现在后缀表达式中，并且会使该后缀表达式以一种特殊方式求值。如果可选链表达式的值为 `nil`，后缀表达式中的所有其他操作都会被忽略，整个后缀表达式求值为 `nil`。如果可选链表达式的值不为 `nil`，就解包该可选链表达式的值，并用它来求值后缀表达式的其余部分。无论哪种情况，后缀表达式的值仍然是可选类型。

如果包含可选链表达式的后缀表达式嵌套在其他后缀表达式中，则只有最外层的表达式返回可选类型。在下面的示例中，当 `c` 不为 `nil` 时，会解包它的值并用它来求值 `.property`，再用得到的值求值 `.performAction()`。整个表达式 `c?.property.performAction()` 的值是可选类型。

```swift
var c: SomeClass?
var result: Bool? = c?.property.performAction()
```

下面的示例展示了不使用可选链时上面示例的行为。

```swift
var result: Bool?
if let unwrappedC = c {
    result = unwrappedC.property.performAction()
}
```

可选链表达式解包后的值可以被修改，既可以修改值本身，也可以给该值的某个成员赋值。如果可选链表达式的值为 `nil`，赋值运算符右侧的表达式不会被求值。例如：

```swift
func someFunctionWithSideEffects() -> Int {
    return 42  // No actual side effects.
}
var someDictionary = ["a": [1, 2, 3], "b": [10, 20]]

someDictionary["not here"]?[0] = someFunctionWithSideEffects()
// someFunctionWithSideEffects isn't evaluated
// someDictionary is still ["a": [1, 2, 3], "b": [10, 20]]

someDictionary["a"]?[0] = someFunctionWithSideEffects()
// someFunctionWithSideEffects is evaluated and returns 42
// someDictionary is now ["a": [42, 2, 3], "b": [10, 20]]
```

> 可选链表达式语法：
>
> *optional-chaining-expression* → *postfix-expression* **`?`**
