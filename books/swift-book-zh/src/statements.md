# 语句 {#Statements}

组合表达式，控制执行流程。

在 Swift 中，语句分为三类：简单语句、编译器控制语句和控制流语句。简单语句最常见，由表达式或声明构成。编译器控制语句让程序可以改变编译器行为的某些方面，包括条件编译块和行控制语句。

控制流语句用来控制程序中的执行流程。Swift 中有几种控制流语句，包括循环语句、分支语句和控制转移语句。循环语句让一块代码可以反复执行，分支语句让某块代码仅在满足特定条件时才执行，控制转移语句则提供了一种改变代码执行顺序的方式。此外，Swift 还提供 `do` 语句来引入作用域并捕获和处理错误，以及 `defer` 语句来在当前作用域退出前执行清理操作。

任何语句后面都可以选择性地出现一个分号（`;`），当多条语句出现在同一行时，用它来分隔。

> 语句语法：
>
> *statement* → *expression* **`;`**_?_ \
> *statement* → *declaration* **`;`**_?_ \
> *statement* → *loop-statement* **`;`**_?_ \
> *statement* → *branch-statement* **`;`**_?_ \
> *statement* → *labeled-statement* **`;`**_?_ \
> *statement* → *control-transfer-statement* **`;`**_?_ \
> *statement* → *defer-statement* **`;`**_?_ \
> *statement* → *do-statement* **`;`**_?_ \
> *statement* → *compiler-control-statement* \
> *statements* → *statement* *statements*_?_

## 循环语句 {#Loop-Statements}

循环语句让一块代码可以根据循环中指定的条件反复执行。Swift 有三种循环语句：`for`-`in` 语句、`while` 语句和 `repeat`-`while` 语句。

循环语句中的控制流可以通过 `break` 语句和 `continue` 语句改变，详见下面的 [break 语句](statements.md#Break-Statement) 和 [continue 语句](statements.md#Continue-Statement)。

> 循环语句语法：
>
> *loop-statement* → *for-in-statement* \
> *loop-statement* → *while-statement* \
> *loop-statement* → *repeat-while-statement*

### for-in 语句 {#For-In-Statement}

`for`-`in` 语句让一块代码对遵循 [`Sequence`](https://developer.apple.com/documentation/swift/sequence) 协议的集合（或任意类型）中的每个元素执行一次。

`for`-`in` 语句的形式如下：

```swift
for <#item#> in <#collection#> {
   <#statements#>
}
```

程序会在*集合*表达式上调用 `makeIterator()` 方法，以获得一个迭代器类型的值——也就是遵循 [`IteratorProtocol`](https://developer.apple.com/documentation/swift/iteratorprotocol) 协议的类型。程序通过调用迭代器的 `next()` 方法开始执行循环。如果返回的值不是 `nil`，就把它赋给*元素*模式，程序执行*语句*，然后回到循环开头继续执行。否则，程序不执行赋值，也不执行*语句*，并结束 `for`-`in` 语句的执行。

> for-in 语句语法：
>
> *for-in-statement* → **`for`** **`case`**_?_ *pattern* **`in`** *expression* *where-clause*_?_ *code-block*

### while 语句 {#While-Statement}

`while` 语句让一块代码在某个条件保持为真时反复执行。

`while` 语句的形式如下：

```swift
while <#condition#> {
   <#statements#>
}
```

`while` 语句的执行过程如下：

1. 求值*条件*。

如果为 `true`，执行继续到第 2 步。如果为 `false`，程序就结束 `while` 语句的执行。
2. 程序执行*语句*，然后执行回到第 1 步。

由于*条件*的值在*语句*执行之前求值，`while` 语句中的*语句*可以执行零次或多次。

*条件*的值必须是 `Bool` 类型或可以桥接到 `Bool` 的类型。条件也可以是一个可选绑定声明，详见 [可选绑定](the-basics.md#Optional-Binding)。

> while 语句语法：
>
> *while-statement* → **`while`** *condition-list* *code-block*
>
> *condition-list* → *condition* | *condition* **`,`** *condition-list* \
> *condition* → *expression* | *availability-condition* | *case-condition* | *optional-binding-condition*
>
> *case-condition* → **`case`** *pattern* *initializer* \
> *optional-binding-condition* → **`let`** *pattern* *initializer*_?_ | **`var`** *pattern* *initializer*_?_

### repeat-while 语句 {#Repeat-While-Statement}

`repeat`-`while` 语句让一块代码在某个条件保持为真时执行一次或多次。

`repeat`-`while` 语句的形式如下：

```swift
repeat {
   <#statements#>
} while <#condition#>
```

`repeat`-`while` 语句的执行过程如下：

1. 程序执行*语句*，然后执行继续到第 2 步。
2. 求值*条件*。

如果为 `true`，执行回到第 1 步。如果为 `false`，程序就结束 `repeat`-`while` 语句的执行。

由于*条件*的值在*语句*执行之后才求值，`repeat`-`while` 语句中的*语句*至少执行一次。

*条件*的值必须是 `Bool` 类型或可以桥接到 `Bool` 的类型。

> repeat-while 语句语法：
>
> *repeat-while-statement* → **`repeat`** *code-block* **`while`** *expression*

## 分支语句 {#Branch-Statements}

分支语句让程序根据一个或多个条件的值执行代码的某些部分。分支语句中指定的条件值控制程序如何分支，因而也控制执行哪一块代码。Swift 有三种分支语句：`if` 语句、`guard` 语句和 `switch` 语句。

`if` 语句或 `switch` 语句中的控制流可以通过 `break` 语句改变，详见下面的 [break 语句](statements.md#Break-Statement)。

> 分支语句语法：
>
> *branch-statement* → *if-statement* \
> *branch-statement* → *guard-statement* \
> *branch-statement* → *switch-statement*

### if 语句 {#If-Statement}

`if` 语句用于根据一个或多个条件的求值结果执行代码。

`if` 语句有两种基本形式。两种形式都要求写左花括号和右花括号。

第一种形式只在条件为真时执行代码，其形式如下：

```swift
if <#condition#> {
   <#statements#>
}
```

`if` 语句的第二种形式提供一个额外的*else 子句*（由 `else` 关键字引入），用于在条件为真时执行一部分代码、在同一条件为假时执行另一部分代码。当只有一个 else 子句时，`if` 语句的形式如下：

```swift
if <#condition#> {
   <#statements to execute if condition is true#>
} else {
   <#statements to execute if condition is false#>
}
```

`if` 语句的 else 子句可以包含另一个 `if` 语句，以测试多个条件。这样串联起来的 `if` 语句形式如下：

```swift
if <#condition 1#> {
   <#statements to execute if condition 1 is true#>
} else if <#condition 2#> {
   <#statements to execute if condition 2 is true#>
} else {
   <#statements to execute if both conditions are false#>
}
```

`if` 语句中任何条件的值都必须是 `Bool` 类型或可以桥接到 `Bool` 的类型。条件也可以是一个可选绑定声明，详见 [可选绑定](the-basics.md#Optional-Binding)。

> if 语句语法：
>
> *if-statement* → **`if`** *condition-list* *code-block* *else-clause*_?_ \
> *else-clause* → **`else`** *code-block* | **`else`** *if-statement*

### guard 语句 {#Guard-Statement}

`guard` 语句用于在一个或多个条件不满足时把程序控制转移出当前作用域。

`guard` 语句的形式如下：

```swift
guard <#condition#> else {
   <#statements#>
}
```

`guard` 语句中任何条件的值都必须是 `Bool` 类型或可以桥接到 `Bool` 的类型。条件也可以是一个可选绑定声明，详见 [可选绑定](the-basics.md#Optional-Binding)。

在 `guard` 语句的条件中通过可选绑定声明赋值的任何常量或变量，都可以在 `guard` 语句所在作用域的其余部分使用。

`guard` 语句的 `else` 子句是必需的，并且必须调用返回类型为 `Never` 的函数，或者使用下列语句之一把程序控制转移到 `guard` 语句所在作用域之外：

- `return`
- `break`
- `continue`
- `throw`

控制转移语句详见下面的 [控制转移语句](statements.md#Control-Transfer-Statements)。关于返回类型为 `Never` 的函数的更多信息，请参阅 [永不返回的函数](declarations.md#Functions-that-Never-Return)。

> guard 语句语法：
>
> *guard-statement* → **`guard`** *condition-list* **`else`** *code-block*

### switch 语句 {#Switch-Statement}

`switch` 语句让某些代码块根据一个控制表达式的值执行。

`switch` 语句的形式如下：

```swift
switch <#control expression#> {
case <#pattern 1#>:
    <#statements#>
case <#pattern 2#> where <#condition#>:
    <#statements#>
case <#pattern 3#> where <#condition#>,
    <#pattern 4#> where <#condition#>:
    <#statements#>
default:
    <#statements#>
}
```

先求值 `switch` 语句的*控制表达式*，然后把它与每个分支中指定的模式进行比较。如果找到匹配项，程序就执行该分支作用域内列出的*语句*。每个分支的作用域不能为空。因此，每个分支标签的冒号（`:`）之后必须至少包含一条语句。如果你不打算在匹配分支的主体中执行任何代码，请使用单个 `break` 语句。

可用于分支的表达式的值非常灵活。例如，除了整数和字符等标量类型的值之外，代码还可以根据任意类型的值分支，包括浮点数、字符串、元组、自定义类的实例和可选值。*控制表达式*的值甚至可以与枚举中某个成员的值匹配，并检查它是否包含在指定的值范围内。关于如何在 `switch` 语句中使用这些不同类型的值，请参阅 [控制流](control-flow.md) 中的 [Switch](control-flow.md#Switch)。

`switch` 的每个分支可以在每个模式之后选择性地包含一个 `where` 子句。*where 子句*由 `where` 关键字后跟一个表达式引入，用于在认定分支中的模式与*控制表达式*匹配之前提供一个附加条件。如果存在 `where` 子句，那么只有当*控制表达式*的值匹配该分支的某个模式，并且 `where` 子句的表达式求值为 `true` 时，才会执行相关分支中的*语句*。例如，下面的例子中，只有当*控制表达式*是一个包含两个相同值的元素的元组（如 `(1, 1)`）时，它才匹配该分支。

```swift
case let (x, y) where x == y:
```

如上面的例子所示，分支中的模式还可以使用 `let` 关键字绑定常量（也可以使用 `var` 关键字绑定变量）。随后，可以在对应的 `where` 子句以及该分支作用域内其余代码中引用这些常量（或变量）。如果该分支包含多个匹配控制表达式的模式，那么所有模式都必须包含相同的常量或变量绑定，并且每个绑定的变量或常量在该分支的所有模式中必须具有相同的类型。

`switch` 语句还可以包含一个默认分支，由 `default` 关键字引入。只有当没有其他分支匹配控制表达式时，才会执行默认分支中的代码。一个 `switch` 语句只能包含一个默认分支，并且它必须出现在 `switch` 语句的末尾。

尽管模式匹配操作的实际执行顺序（尤其是各分支中模式的求值顺序）并未规定，但 `switch` 语句中的模式匹配表现得就像按源码顺序求值一样——也就是它们在源代码中出现的顺序。因此，如果多个分支包含求值结果相同的模式，因而都能匹配控制表达式的值，程序只会执行源码顺序中第一个匹配分支内的代码。

#### switch 语句必须是穷尽的 {#Switch-Statements-Must-Be-Exhaustive}

在 Swift 中，控制表达式类型的每个可能值都必须与至少一个分支的模式相匹配。如果这根本无法做到（例如控制表达式的类型是 `Int` 时），你可以包含一个默认分支来满足这一要求。

#### 对将来新增的枚举成员进行切换 {#Switching-Over-Future-Enumeration-Cases}

*非冻结枚举*（nonfrozen enumeration）是一种特殊的枚举，将来可能会增加新的枚举成员——即使在你编译并发布 App 之后也是如此。对非冻结枚举进行切换需要额外考虑。当库的作者把枚举标记为非冻结时，他们就保留了添加新枚举成员的权利，任何与该枚举交互的代码都*必须*能够在不重新编译的情况下处理这些未来的成员。在库演进模式下编译的代码、Swift 标准库中的代码、针对 Apple 框架的 Swift overlay，以及 C 和 Objective-C 代码都可以声明非冻结枚举。关于冻结和非冻结枚举的信息，请参阅 [frozen](attributes.md#frozen)。

对非冻结枚举值进行切换时，你始终需要包含一个默认分支，即使该枚举的每个成员都已有对应的 switch 分支。你可以把 `@unknown` 特性应用于默认分支，表示该默认分支只应匹配将来添加的枚举成员。如果默认分支匹配了编译时已知的任何枚举成员，Swift 会产生一条警告。这条针对未来情况的警告会通知你：库作者为枚举添加了一个没有对应 switch 分支的新成员。

下面的例子对 Swift 标准库 [`Mirror.AncestorRepresentation`](https://developer.apple.com/documentation/swift/mirror/ancestorrepresentation) 枚举现有的全部三个成员进行切换。如果将来添加了其他成员，编译器会生成一条警告，指出你需要更新该 switch 语句以考虑这些新成员。

```swift
let representation: Mirror.AncestorRepresentation = .generated
switch representation {
case .customized:
    print("Use the nearest ancestor’s implementation.")
case .generated:
    print("Generate a default mirror for all ancestor classes.")
case .suppressed:
    print("Suppress the representation of all ancestor classes.")
@unknown default:
    print("Use a representation that was unknown when this code was compiled.")
}
// Prints "Generate a default mirror for all ancestor classes."
```

#### 执行不会隐式贯穿分支 {#Execution-Does-Not-Fall-Through-Cases-Implicitly}

匹配分支中的代码执行完毕后，程序就退出 `switch` 语句。程序执行不会继续或“贯穿”到下一个分支或默认分支。话虽如此，如果你想让执行从一个分支继续到下一个分支，可以在希望继续执行的那个分支中显式包含一个 `fallthrough` 语句，它只由 `fallthrough` 关键字构成。关于 `fallthrough` 语句的更多信息，详见下面的 [fallthrough 语句](statements.md#Fallthrough-Statement)。

> switch 语句语法：
>
> *switch-statement* → **`switch`** *expression* **`{`** *switch-cases*_?_ **`}`** \
> *switch-cases* → *switch-case* *switch-cases*_?_ \
> *switch-case* → *case-label* *statements* \
> *switch-case* → *default-label* *statements* \
> *switch-case* → *conditional-switch-case*
>
> *case-label* → *attributes*_?_ **`case`** *case-item-list* **`:`** \
> *case-item-list* → *pattern* *where-clause*_?_ | *pattern* *where-clause*_?_ **`,`** *case-item-list* \
> *default-label* → *attributes*_?_ **`default`** **`:`**
>
> *where-clause* → **`where`** *where-expression* \
> *where-expression* → *expression*
>
> *conditional-switch-case* → *switch-if-directive-clause* *switch-elseif-directive-clauses*_?_ *switch-else-directive-clause*_?_ *endif-directive* \
> *switch-if-directive-clause* → *if-directive* *compilation-condition* *switch-cases*_?_ \
> *switch-elseif-directive-clauses* → *elseif-directive-clause* *switch-elseif-directive-clauses*_?_ \
> *switch-elseif-directive-clause* → *elseif-directive* *compilation-condition* *switch-cases*_?_ \
> *switch-else-directive-clause* → *else-directive* *switch-cases*_?_

## 带标签的语句 {#Labeled-Statement}

你可以给循环语句、`if` 语句、`switch` 语句或 `do` 语句加上*语句标签*（statement label）作为前缀，语句标签由标签名紧跟一个冒号（:）构成。把语句标签与 `break` 和 `continue` 语句一起使用，可以明确表达你想如何改变循环语句或 `switch` 语句中的控制流，详见下面的 [break 语句](statements.md#Break-Statement) 和 [continue 语句](statements.md#Continue-Statement)。

带标签的语句的作用域是语句标签之后的整个语句。你可以嵌套带标签的语句，但每个语句标签的名称必须唯一。

关于语句标签用法的更多信息和示例，请参阅 [控制流](control-flow.md) 中的 [带标签的语句](control-flow.md#Labeled-Statements)。

> 带标签的语句语法：
>
> *labeled-statement* → *statement-label* *loop-statement* \
> *labeled-statement* → *statement-label* *if-statement* \
> *labeled-statement* → *statement-label* *switch-statement* \
> *labeled-statement* → *statement-label* *do-statement*
>
> *statement-label* → *label-name* **`:`** \
> *label-name* → *identifier*

## 控制转移语句 {#Control-Transfer-Statements}

控制转移语句可以把程序控制从一段代码无条件地转移到另一段代码，从而改变程序中代码的执行顺序。Swift 有五种控制转移语句：`break` 语句、`continue` 语句、`fallthrough` 语句、`return` 语句和 `throw` 语句。

> 控制转移语句语法：
>
> *control-transfer-statement* → *break-statement* \
> *control-transfer-statement* → *continue-statement* \
> *control-transfer-statement* → *fallthrough-statement* \
> *control-transfer-statement* → *return-statement* \
> *control-transfer-statement* → *throw-statement*

### break 语句 {#Break-Statement}

`break` 语句结束循环、`if` 语句或 `switch` 语句的执行。`break` 语句可以只由 `break` 关键字构成，也可以由 `break` 关键字后跟一个语句标签名构成，如下所示。

```swift
break
break <#label name#>
```

当 `break` 语句后跟语句标签名时，它结束该标签所指定的循环、`if` 语句或 `switch` 语句的执行。

当 `break` 语句没有后跟语句标签名时，它结束其所在的 `switch` 语句或最内层外围循环语句的执行。不能使用不带标签的 `break` 语句跳出 `if` 语句。

在两种情况下，程序控制随后都会转移到外围循环或 `switch` 语句之后的第一行代码（如果有的话）。

关于 `break` 语句用法的示例，请参阅 [控制流](control-flow.md) 中的 [Break](control-flow.md#Break) 和 [带标签的语句](control-flow.md#Labeled-Statements)。

> break 语句语法：
>
> *break-statement* → **`break`** *label-name*_?_

### continue 语句 {#Continue-Statement}

`continue` 语句结束循环语句当前这一次迭代的执行，但不会停止该循环语句的执行。`continue` 语句可以只由 `continue` 关键字构成，也可以由 `continue` 关键字后跟一个语句标签名构成，如下所示。

```swift
continue
continue <#label name#>
```

当 `continue` 语句后跟语句标签名时，它结束该标签所指定的循环语句当前这一次迭代的执行。

当 `continue` 语句没有后跟语句标签名时，它结束其所在的最内层外围循环语句当前这一次迭代的执行。

在两种情况下，程序控制随后都会转移到外围循环语句的条件处。

在 `for` 语句中，`continue` 语句执行之后仍会求值递增表达式，因为递增表达式是在循环体执行之后求值的。

关于 `continue` 语句用法的示例，请参阅 [控制流](control-flow.md) 中的 [Continue](control-flow.md#Continue) 和 [带标签的语句](control-flow.md#Labeled-Statements)。

> continue 语句语法：
>
> *continue-statement* → **`continue`** *label-name*_?_

### fallthrough 语句 {#Fallthrough-Statement}

`fallthrough` 语句由 `fallthrough` 关键字构成，只出现在 `switch` 语句的分支代码块中。`fallthrough` 语句使程序执行从 `switch` 语句的一个分支继续到下一个分支。即使分支标签的模式不匹配 `switch` 语句控制表达式的值，程序执行也会继续到下一个分支。

`fallthrough` 语句可以出现在 `switch` 语句内部的任何位置，而不只是作为分支代码块的最后一条语句，但它不能用在最后一个分支代码块中。它也不能把控制转移到模式包含值绑定模式的分支代码块中。

关于在 `switch` 语句中使用 `fallthrough` 语句的示例，请参阅 [控制流](control-flow.md) 中的 [控制转移语句](control-flow.md#Control-Transfer-Statements)。

> fallthrough 语句语法：
>
> *fallthrough-statement* → **`fallthrough`**

### return 语句 {#Return-Statement}

`return` 语句出现在函数或方法定义的主体中，使程序执行返回到调用它的函数或方法。程序执行从紧接函数或方法调用之后的位置继续。

`return` 语句可以只由 `return` 关键字构成，也可以由 `return` 关键字后跟一个表达式构成，如下所示。

```swift
return
return <#expression#>
```

当 `return` 语句后跟一个表达式时，该表达式的值会返回给调用它的函数或方法。如果表达式的值与函数或方法声明中指定的返回类型不匹配，那么表达式的值会在返回给调用方之前转换为该返回类型。

> 注意：如 [可失败构造器](declarations.md#Failable-Initializers) 所述，`return` 语句的一种特殊形式（`return nil`）可以用在可失败构造器中，表示构造失败。

当 `return` 语句没有后跟表达式时，它只能用于从不返回值的函数或方法中返回（也就是说，该函数或方法的返回类型是 `Void` 或 `()`）。

> return 语句语法：
>
> *return-statement* → **`return`** *expression*_?_

### throw 语句 {#Throw-Statement}

`throw` 语句出现在抛出函数或方法的主体中，或者出现在类型标有 `throws` 关键字的闭包表达式的主体中。

`throw` 语句使程序结束当前作用域的执行，并开始向周围作用域传播错误。抛出的错误会继续传播，直到被 `do` 语句的某个 `catch` 子句处理。

`throw` 语句由 `throw` 关键字后跟一个表达式构成，如下所示。

```swift
throw <#expression#>
```

*表达式*的值必须具有遵循 `Error` 协议的类型。如果包含该 `throw` 语句的 `do` 语句或函数声明了它所抛出的错误类型，那么*表达式*的值必须是该类型的实例。

关于 `throw` 语句用法的示例，请参阅 [错误处理](error-handling.md) 中的 [用抛出函数传播错误](error-handling.md#Propagating-Errors-Using-Throwing-Functions)。

> throw 语句语法：
>
> *throw-statement* → **`throw`** *expression*

## defer 语句 {#Defer-Statement}

`defer` 语句用于在程序控制即将离开 `defer` 语句所在的作用域之前执行代码。

`defer` 语句的形式如下：

```swift
defer {
    <#statements#>
}
```

无论程序控制以何种方式转移，`defer` 语句中的语句都会执行。这意味着 `defer` 语句可以用来执行手动资源管理，例如关闭文件描述符，也可以用来执行即使抛出错误也必须发生的操作。

`defer` 语句中的*语句*在包含该 `defer` 语句的作用域结束时执行。

```swift
func f(x: Int) {
  defer { print("First defer") }

  if x < 10 {
    defer { print("Second defer") }
    print("End of if")
  }

  print("End of function")
}
f(x: 5)
// Prints "End of if".
// Prints "Second defer".
// Prints "End of function".
// Prints "First defer".
```

在上面的代码中，`if` 语句中的 `defer` 会先于函数 `f` 中声明的 `defer` 执行，因为 `if` 语句的作用域先于函数作用域结束。

如果同一个作用域中出现多个 `defer` 语句，它们出现的顺序与执行的顺序相反。先执行给定作用域中的最后一个 `defer` 语句，意味着该最后一个 `defer` 语句内部的语句可以引用将被其他 `defer` 语句清理的资源。

```swift
func f() {
    defer { print("First defer") }
    defer { print("Second defer") }
    print("End of function")
}
f()
// Prints "End of function".
// Prints "Second defer".
// Prints "First defer".
```

`defer` 语句中的语句不能把程序控制转移到 `defer` 语句之外。

> defer 语句语法：
>
> *defer-statement* → **`defer`** *code-block*

## do 语句 {#Do-Statement}

`do` 语句用于引入一个新的作用域，并且可以选择性地包含一个或多个 `catch` 子句，其中包含与已定义的错误条件相匹配的模式。在 `do` 语句的作用域中声明的变量和常量只能在该作用域内访问。

Swift 中的 `do` 语句类似于 C 中用于界定代码块的花括号（`{}`），并且不会在运行时带来性能开销。

`do` 语句的形式如下：

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

`do` 语句可以选择性地指定它所抛出的错误类型，其形式如下：

```swift
do throws(<#type#>) {
    try <#expression#>
} catch <#pattern> {
    <#statements#>
} catch {
    <#statements#>
}
```

如果 `do` 语句包含 `throws` 子句，则 `do` 块只能抛出指定*类型*的错误。该*类型*必须是遵循 `Error` 协议的具体类型、遵循 `Error` 协议的不透明类型，或者是装箱协议类型 `any Error`。如果 `do` 语句没有指定它所抛出的错误类型，Swift 会按如下方式推断错误类型：

- 如果 `do` 代码块中的每个 `throws` 语句和 `try` 表达式都嵌套在穷尽的错误处理机制内，Swift 就推断该 `do` 语句不抛出错误。

- 如果 `do` 代码块中包含在穷尽错误处理之外只抛出单一类型错误的代码（抛出 `Never` 除外），Swift 就推断该 `do` 语句抛出该具体错误类型。

- 如果 `do` 代码块中包含在穷尽错误处理之外抛出不止一种类型错误的代码，Swift 就推断该 `do` 语句抛出 `any Error`。

关于处理具有显式类型的错误的更多信息，请参阅 [指定错误类型](error-handling.md#Specifying-the-Error-Type)。

如果 `do` 代码块中的任何语句抛出错误，程序控制会转移到模式与该错误匹配的第一个 `catch` 子句。如果没有任何子句匹配，错误会传播到周围作用域。如果错误在顶层未被处理，程序执行会因运行时错误而停止。

与 `switch` 语句一样，编译器会尝试推断 `catch` 子句是否穷尽。如果可以作出这样的判断，就认为该错误已得到处理。否则，错误可以传播到包含它的作用域之外，这意味着该错误必须由外围的 `catch` 子句处理，或者包含它的函数必须声明为 `throws`。

具有多个模式的 `catch` 子句只要其中任一模式与错误匹配，就算匹配该错误。如果 `catch` 子句包含多个模式，那么所有模式都必须包含相同的常量或变量绑定，并且每个绑定的变量或常量在该 `catch` 子句的所有模式中必须具有相同的类型。

要确保错误得到处理，请使用模式能匹配所有错误的 `catch` 子句，例如通配符模式（`_`）。如果 `catch` 子句没有指定模式，该 `catch` 子句会匹配任何错误，并把该错误绑定到名为 `error` 的局部常量。关于 `catch` 子句中可使用的模式的更多信息，请参阅 [模式](patterns.md)。

关于如何使用带有多个 `catch` 子句的 `do` 语句的示例，请参阅 [处理错误](error-handling.md#Handling-Errors)。

> do 语句语法：
>
> *do-statement* → **`do`** *throws-clause*_?_ *code-block* *catch-clauses*_?_ \
> *catch-clauses* → *catch-clause* *catch-clauses*_?_ \
> *catch-clause* → **`catch`** *catch-pattern-list*_?_ *code-block* \
> *catch-pattern-list* → *catch-pattern* | *catch-pattern* **`,`** *catch-pattern-list* \
> *catch-pattern* → *pattern* *where-clause*_?_

## 编译器控制语句 {#Compiler-Control-Statements}

编译器控制语句让程序可以改变编译器行为的某些方面。Swift 有三种编译器控制语句：条件编译块、行控制语句和编译时诊断语句。

> 编译器控制语句语法：
>
> *compiler-control-statement* → *conditional-compilation-block* \
> *compiler-control-statement* → *line-control-statement* \
> *compiler-control-statement* → *diagnostic-statement*

### 条件编译块 {#Conditional-Compilation-Block}

条件编译块让代码可以根据一个或多个编译条件的值有条件地编译。

每个条件编译块都以 `#if` 编译指令开始，以 `#endif` 编译指令结束。简单的条件编译块形式如下：

```swift
#if <#compilation condition#>
    <#statements#>
#endif
```

与 `if` 语句的条件不同，*编译条件*在编译时求值。因此，只有当*编译条件*在编译时求值为 `true` 时，*语句*才会被编译和执行。

*编译条件*可以包含 `true` 和 `false` 布尔字面量、与 `-D` 命令行标志一起使用的标识符，或下表中列出的任何平台条件。

| 平台条件 | 有效实参 |
| ------------------ | --------------- |
| `os()` | `macOS`, `iOS`, `watchOS`, `tvOS`, `visionOS`, `Linux`, `Windows` |
| `arch()` | `arm`, `arm64`, `i386`, `wasm32`, `x86_64`, |
| `swift()` | `>=` 或 `<` 后跟一个版本号 |
| `compiler()` | `>=` 或 `<` 后跟一个版本号 |
| `canImport()` | 模块名 |
| `targetEnvironment()` | `simulator`, `macCatalyst` |

`swift()` 和 `compiler()` 平台条件的版本号由主版本号、可选的小版本号、可选的补丁版本号等组成，版本号的每一部分之间用点（`.`）分隔。比较运算符与版本号之间不能有空白。`compiler()` 的版本是编译器版本，与传给编译器的 Swift 版本设置无关。`swift()` 的版本是当前正在编译的语言版本。例如，如果你用 Swift 5 编译器以 Swift 4.2 模式编译代码，编译器版本是 5，语言版本是 4.2。在这些设置下，下面的代码会打印全部三条消息：

```swift
#if compiler(>=5)
print("Compiled with the Swift 5 compiler or later")
#endif
#if swift(>=4.2)
print("Compiled in Swift 4.2 mode or later")
#endif
#if compiler(>=5) && swift(<5)
print("Compiled with the Swift 5 compiler or later in a Swift mode earlier than 5")
#endif
// Prints "Compiled with the Swift 5 compiler or later".
// Prints "Compiled in Swift 4.2 mode or later".
// Prints "Compiled with the Swift 5 compiler or later in a Swift mode earlier than 5".
```

`canImport()` 平台条件的实参是一个模块名，该模块可能并非在所有平台上都存在。模块名中可以包含句点（`.`）。这个条件测试是否可以导入该模块，但并不会实际导入它。如果模块存在，该平台条件返回 `true`；否则返回 `false`。

当代码针对指定环境编译时，`targetEnvironment()` 平台条件返回 `true`；否则返回 `false`。

> 注意：对于 ARM 64 设备，`arch(arm)` 平台条件不返回 `true`。当代码针对 32 位 iOS 模拟器编译时，`arch(i386)` 平台条件返回 `true`。

你可以使用逻辑运算符 `&&`、`||` 和 `!` 组合和取反编译条件，并使用圆括号分组。这些运算符的结合性和优先级与用于组合普通布尔表达式的逻辑运算符相同。

与 `if` 语句类似，你可以添加多个条件分支来测试不同的编译条件。你可以使用 `#elseif` 子句添加任意数量的附加分支。你还可以使用 `#else` 子句添加最后一个附加分支。包含多个分支的条件编译块形式如下：

```swift
#if <#compilation condition 1#>
    <#statements to compile if compilation condition 1 is true#>
#elseif <#compilation condition 2#>
    <#statements to compile if compilation condition 2 is true#>
#else
    <#statements to compile if both compilation conditions are false#>
#endif
```

> 注意：条件编译块主体中的每条语句都会被解析，即使它不会被编译。不过有一个例外：如果编译条件包含 `swift()` 或 `compiler()` 平台条件，那么只有当语言或编译器版本与平台条件中指定的版本匹配时，这些语句才会被解析。这个例外确保较旧的编译器不会尝试解析较新版本 Swift 引入的语法。

关于如何在条件编译块中包装显式成员表达式的信息，请参阅 [显式成员表达式](expressions.md#Explicit-Member-Expression)。

> 条件编译块语法：
>
> *conditional-compilation-block* → *if-directive-clause* *elseif-directive-clauses*_?_ *else-directive-clause*_?_ *endif-directive*
>
> *if-directive-clause* → *if-directive* *compilation-condition* *statements*_?_ \
> *elseif-directive-clauses* → *elseif-directive-clause* *elseif-directive-clauses*_?_ \
> *elseif-directive-clause* → *elseif-directive* *compilation-condition* *statements*_?_ \
> *else-directive-clause* → *else-directive* *statements*_?_ \
> *if-directive* → **`#if`** \
> *elseif-directive* → **`#elseif`** \
> *else-directive* → **`#else`** \
> *endif-directive* → **`#endif`**
>
> *compilation-condition* → *platform-condition* \
> *compilation-condition* → *identifier* \
> *compilation-condition* → *boolean-literal* \
> *compilation-condition* → **`(`** *compilation-condition* **`)`** \
> *compilation-condition* → **`!`** *compilation-condition* \
> *compilation-condition* → *compilation-condition* **`&&`** *compilation-condition* \
> *compilation-condition* → *compilation-condition* **`||`** *compilation-condition*
>
> *platform-condition* → **`os`** **`(`** *operating-system* **`)`** \
> *platform-condition* → **`arch`** **`(`** *architecture* **`)`** \
> *platform-condition* → **`swift`** **`(`** **`>=`** *swift-version* **`)`** | **`swift`** **`(`** **`<`** *swift-version* **`)`** \
> *platform-condition* → **`compiler`** **`(`** **`>=`** *swift-version* **`)`** | **`compiler`** **`(`** **`<`** *swift-version* **`)`** \
> *platform-condition* → **`canImport`** **`(`** *import-path* **`)`** \
> *platform-condition* → **`targetEnvironment`** **`(`** *environment* **`)`**
>
> *operating-system* → **`macOS`** | **`iOS`** | **`watchOS`** | **`tvOS`** | **`visionOS`** | **`Linux`** | **`Windows`** \
> *architecture* → **`arm`** | **`arm64`** | **`i386`** | **`wasm32`** | **`x86_64`** \
> *swift-version* → *decimal-digits* *swift-version-continuation*_?_ \
> *swift-version-continuation* → **`.`** *decimal-digits* *swift-version-continuation*_?_ \
> *environment* → **`simulator`** | **`macCatalyst`**

### 行控制语句 {#Line-Control-Statement}

行控制语句用于指定与正在编译的源代码不同的行号和文件名。使用行控制语句可以改变 Swift 用于诊断和调试的源代码位置。

行控制语句的形式如下：

```swift
#sourceLocation(file: <#file path#>, line: <#line number#>)
#sourceLocation()
```

行控制语句的第一种形式会改变 `#line`、`#file`、`#fileID` 和 `#filePath` 字面量表达式的值，从该行控制语句之后的下一行代码开始生效。*行号*会改变 `#line` 的值，它可以是任何大于零的整数字面量。*文件路径*会改变 `#file`、`#fileID` 和 `#filePath` 的值，它是一个字符串字面量。指定的字符串会成为 `#filePath` 的值，而 `#fileID` 的值则使用该字符串的最后一个路径分量。关于 `#file`、`#fileID` 和 `#filePath` 的信息，请参阅 [字面量表达式](expressions.md#Literal-Expression)。

行控制语句的第二种形式 `#sourceLocation()` 会把源代码位置重置为默认的行号和文件路径。

> 行控制语句语法：
>
> *line-control-statement* → **`#sourceLocation`** **`(`** **`file:`** *file-path* **`,`** **`line:`** *line-number* **`)`** \
> *line-control-statement* → **`#sourceLocation`** **`(`** **`)`** \
> *line-number* → A decimal integer greater than zero \
> *file-path* → *static-string-literal*

### 编译时诊断语句 {#Compile-Time-Diagnostic-Statement}

在 Swift 5.9 之前，`#warning` 和 `#error` 语句会在编译期间发出诊断信息。现在这一行为由 Swift 标准库中的 [`warning(_:)`][] 和 [`error(_:)`][] 宏提供。

[`warning(_:)`]: https://developer.apple.com/documentation/swift/warning(_:) [`error(_:)`]: https://developer.apple.com/documentation/swift/error(_:)

## 可用性条件 {#Availability-Condition}

*可用性条件*（availability condition）用作 `if`、`while` 和 `guard` 语句的条件，根据指定的平台实参在运行时查询 API 的可用性。

可用性条件的形式如下：

```swift
if #available(<#platform name#> <#version#>, <#...#>, *) {
    <#statements to execute if the APIs are available#>
} else {
    <#fallback statements to execute if the APIs are unavailable#>
}
```

你可以根据要使用的 API 在运行时是否可用，用可用性条件来执行一个代码块。编译器在验证该代码块中的 API 是否可用时，会使用可用性条件中的信息。

可用性条件接受一个以逗号分隔的平台名称和版本列表。平台名称使用 `iOS`、`macOS`、`watchOS`、`tvOS` 和 `visionOS`，并包含相应的版本号。`*` 实参是必需的，它指定在任何其他平台上，由可用性条件守护的代码块主体会在你的目标所指定的最低部署目标上执行。

与布尔条件不同，你不能使用 `&&` 和 `||` 这样的逻辑运算符组合可用性条件。要取反可用性条件，不要使用 `!`，而应使用不可用性条件，其形式如下：

```swift
if #unavailable(<#platform name#> <#version#>, <#...#>) {
    <#fallback statements to execute if the APIs are unavailable#>
} else {
    <#statements to execute if the APIs are available#>
}
```

`#unavailable` 形式是取反该条件的语法糖。在不可用性条件中，`*` 实参是隐式的，不得包含。它的含义与可用性条件中的 `*` 实参相同。

> 可用性条件语法：
>
> *availability-condition* → **`#available`** **`(`** *availability-arguments* **`)`** \
> *availability-condition* → **`#unavailable`** **`(`** *availability-arguments* **`)`** \
> *availability-arguments* → *availability-argument* | *availability-argument* **`,`** *availability-arguments* \
> *availability-argument* → *platform-name* *platform-version* \
> *availability-argument* → **`*`**
>
> *platform-name* → **`iOS`** | **`iOSApplicationExtension`** \
> *platform-name* → **`macOS`** | **`macOSApplicationExtension`** \
> *platform-name* → **`macCatalyst`** | **`macCatalystApplicationExtension`** \
> *platform-name* → **`watchOS`** | **`watchOSApplicationExtension`** \
> *platform-name* → **`tvOS`** | **`tvOSApplicationExtension`** \
> *platform-name* → **`visionOS`** | **`visionOSApplicationExtension`** \
> *platform-version* → *decimal-digits* \
> *platform-version* → *decimal-digits* **`.`** *decimal-digits* \
> *platform-version* → *decimal-digits* **`.`** *decimal-digits* **`.`** *decimal-digits*
