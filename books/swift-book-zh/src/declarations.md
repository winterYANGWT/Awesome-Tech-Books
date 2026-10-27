# 声明 {#Declarations}

介绍类型、运算符、变量以及其他名称和构造。

*声明*（declaration）向你的程序中引入新的名称或构造。例如，你可以用声明引入函数和方法，引入变量和常量，以及定义枚举、结构体、类和协议类型。你还可以用声明扩展已有具名类型的行为，并把其他地方声明的符号导入你的程序。

在 Swift 中，大多数声明同时也是定义，因为它们在声明的同时就被实现或初始化。话虽如此，由于协议并不实现其成员，大多数协议成员只是声明。为方便起见，也因为在 Swift 中这种区别并不那么重要，*声明*一词同时涵盖声明和定义。

> 声明语法：
>
> *declaration* → *import-declaration* \
> *declaration* → *constant-declaration* \
> *declaration* → *variable-declaration* \
> *declaration* → *typealias-declaration* \
> *declaration* → *function-declaration* \
> *declaration* → *enum-declaration* \
> *declaration* → *struct-declaration* \
> *declaration* → *class-declaration* \
> *declaration* → *actor-declaration* \
> *declaration* → *protocol-declaration* \
> *declaration* → *initializer-declaration* \
> *declaration* → *deinitializer-declaration* \
> *declaration* → *extension-declaration* \
> *declaration* → *subscript-declaration* \
> *declaration* → *macro-declaration* \
> *declaration* → *operator-declaration* \
> *declaration* → *precedence-group-declaration*

## 顶层代码 {#Top-Level-Code}

Swift 源文件中的顶层代码由零个或多个语句、声明和表达式组成。默认情况下，同一个模块中每个源文件的代码都可以访问在源文件顶层声明的变量、常量和其他具名声明。你可以用访问级别修饰符标记声明来覆盖这一默认行为，详见 [访问控制级别](declarations.md#Access-Control-Levels)。

顶层代码有两种：顶层声明和可执行顶层代码。顶层声明只由声明组成，所有 Swift 源文件都允许出现。可执行顶层代码包含语句和表达式，而不只是声明，只允许作为程序的顶层入口点。

无论代码如何组织到文件和模块中，你编译成可执行文件的 Swift 代码最多只能采用以下一种方式来标记顶层入口点：包含顶层可执行代码的文件、`main.swift` 文件、`main` 特性、`NSApplicationMain` 特性或 `UIApplicationMain` 特性。

> 顶层声明语法：
>
> *top-level-declaration* → *statements*_?_

## 代码块 {#Code-Blocks}

*代码块*被各种声明和控制结构用来把语句组合在一起，形式如下：

```swift
{
   <#statements#>
}
```

代码块内的*语句*包括声明、表达式以及其他类型的语句，它们按在源代码中出现的顺序执行。

> 代码块语法：
>
> *code-block* → **`{`** *statements*_?_ **`}`**

## 导入声明 {#Import-Declaration}

*导入声明*让你访问在当前文件之外声明的符号。基本形式会导入整个模块，由 `import` 关键字后跟模块名组成：

```swift
import <#module#>
```

提供更多细节可以限定导入哪些符号——你可以指定某个特定的子模块，或者模块或子模块中的某个特定声明。使用这种详细形式时，只有被导入的符号（而不是声明它的模块）会在当前作用域中可用。

```swift
import <#import kind#> <#module#>.<#symbol name#>
import <#module#>.<#submodule#>
```

> 导入声明语法：
>
> *import-declaration* → *attributes*_?_ **`import`** *import-kind*_?_ *import-path*
>
> *import-kind* → **`typealias`** | **`struct`** | **`class`** | **`enum`** | **`protocol`** | **`let`** | **`var`** | **`func`** \
> *import-path* → *identifier* | *identifier* **`.`** *import-path*

## 常量声明 {#Constant-Declaration}

*常量声明*向你的程序中引入一个常量具名值。常量声明使用 `let` 关键字声明，形式如下：

```swift
let <#constant name#>: <#type#> = <#expression#>
```

常量声明在*常量名称*和初始化*表达式*的值之间建立不可变的绑定；常量的值一旦设定，就不能更改。话虽如此，如果常量是用类对象初始化的，对象本身可以改变，但常量名称与该对象之间的绑定不能改变。

在全局作用域声明常量时，必须用值来初始化它。常量声明出现在函数或方法中时，可以稍后再初始化，只要保证在第一次读取其值之前已经设定值即可。如果编译器能证明该常量的值从未被读取，那么这个常量根本不需要设定值。这种分析称为*确定初始化*（definite initialization）——编译器证明某个值在被读取之前一定已经设定。

> 注意：确定初始化无法构造需要领域知识的证明，而且它跨条件语句跟踪状态的能力有限。如果你能确定某个常量总是有值，但编译器无法证明这一点，可以尝试简化设置该值的代码路径，或者改用变量声明。

常量声明出现在类或结构体声明中时，它被视为*常量属性*。常量声明不是计算属性，因此没有 getter 或 setter。

如果常量声明的*常量名称*是元组模式，那么元组中每一项的名称都会绑定到初始化*表达式*中对应的值。

```swift
let (firstNumber, secondNumber) = (10, 42)
```

在这个示例中，`firstNumber` 是值 `10` 的具名常量，`secondNumber` 是值 `42` 的具名常量。现在这两个常量可以独立使用：

```swift
print("The first number is \(firstNumber).")
// Prints "The first number is 10."
print("The second number is \(secondNumber).")
// Prints "The second number is 42."
```

当*常量名称*的类型可以推断时，常量声明中的类型标注（`:` *type*）是可选的，详见 [类型推断](types.md#Type-Inference)。

要声明常量类型属性，请用 `static` 声明修饰符标记该声明。类的常量类型属性总是隐式地为 final；你不能用 `class` 或 `final` 声明修饰符标记它，来允许或禁止子类重写。类型属性在 [类型属性](properties.md#Type-Properties) 中讨论。

关于常量的更多信息以及何时使用常量的指导，参见 [常量和变量](the-basics.md#Constants-and-Variables) 和 [存储属性](properties.md#Stored-Properties)。

> 常量声明语法：
>
> *constant-declaration* → *attributes*_?_ *declaration-modifiers*_?_ **`let`** *pattern-initializer-list*
>
> *pattern-initializer-list* → *pattern-initializer* | *pattern-initializer* **`,`** *pattern-initializer-list* \
> *pattern-initializer* → *pattern* *initializer*_?_ \
> *initializer* → **`=`** *expression*

## 变量声明 {#Variable-Declaration}

*变量声明*向你的程序中引入一个变量具名值，使用 `var` 关键字声明。

变量声明有多种形式，用于声明不同种类的具名可变值，包括存储变量和计算变量、存储变量属性和计算属性、存储变量观察器和属性观察器，以及静态变量属性。使用哪种形式取决于变量声明所在的作用域，以及你打算声明的变量种类。

> 注意：你也可以在协议声明的上下文中声明属性，详见 [协议属性声明](declarations.md#Protocol-Property-Declaration)。

你可以用 `override` 声明修饰符标记子类的属性声明，从而在子类中重写属性，详见 [重写](inheritance.md#Overriding)。

### 存储变量和存储变量属性 {#Stored-Variables-and-Stored-Variable-Properties}

以下形式声明存储变量或存储变量属性：

```swift
var <#variable name#>: <#type#> = <#expression#>
```

这种形式的变量声明可以定义在全局作用域、函数的局部作用域，或者类或结构体声明的上下文中。当这种形式的变量声明位于全局作用域或函数的局部作用域时，它被称为*存储变量*。当它位于类或结构体声明的上下文中时，它被称为*存储变量属性*。

初始化*表达式*不能出现在协议声明中，但在其他所有上下文中，初始化*表达式*都是可选的。话虽如此，如果没有初始化*表达式*，变量声明就必须包含显式的类型标注（`:` *type*）。

与常量声明一样，如果变量声明省略了初始化*表达式*，那么变量必须在第一次被读取之前设定值。同样与常量声明一样，如果*变量名称*是元组模式，元组中每一项的名称都会绑定到初始化*表达式*中对应的值。

顾名思义，存储变量或存储变量属性的值存储在内存中。

### 计算变量和计算属性 {#Computed-Variables-and-Computed-Properties}

以下形式声明计算变量或计算属性：

```swift
var <#variable name#>: <#type#> {
   get {
      <#statements#>
   }
   set(<#setter name#>) {
      <#statements#>
   }
}
```

这种形式的变量声明可以定义在全局作用域、函数的局部作用域，或者类、结构体、枚举或扩展声明的上下文中。当这种形式的变量声明位于全局作用域或函数的局部作用域时，它被称为*计算变量*。当它位于类、结构体或扩展声明的上下文中时，它被称为*计算属性*。

getter 用于读取值，setter 用于写入值。setter 子句是可选的，当只需要 getter 时，你可以同时省略这两个子句，直接返回所请求的值，详见 [只读计算属性](properties.md#Read-Only-Computed-Properties)。但如果你提供了 setter 子句，就必须同时提供 getter 子句。

*setter 名称*及其外层括号是可选的。如果你提供了 setter 名称，它会被用作 setter 的形参名称。如果不提供 setter 名称，setter 的默认形参名是 `newValue`，详见 [简写 setter 声明](properties.md#Shorthand-Setter-Declaration)。

与存储具名值和存储变量属性不同，计算具名值或计算属性的值并不存储在内存中。

更多信息以及计算属性的示例，参见 [计算属性](properties.md#Computed-Properties)。

### 存储变量观察器和属性观察器 {#Stored-Variable-Observers-and-Property-Observers}

你还可以声明带 `willSet` 和 `didSet` 观察器的存储变量或属性。带观察器的存储变量或属性形式如下：

```swift
var <#variable name#>: <#type#> = <#expression#> {
   willSet(<#setter name#>) {
      <#statements#>
   }
   didSet(<#setter name#>) {
      <#statements#>
   }
}
```

这种形式的变量声明可以定义在全局作用域、函数的局部作用域，或者类或结构体声明的上下文中。当这种形式的变量声明位于全局作用域或函数的局部作用域时，这些观察器被称为*存储变量观察器*。当它位于类或结构体声明的上下文中时，这些观察器被称为*属性观察器*。

你可以为任何存储属性添加属性观察器。你也可以通过在子类中重写属性，为任何继承来的属性（无论是存储属性还是计算属性）添加属性观察器，详见 [重写属性观察器](inheritance.md#Overriding-Property-Observers)。

初始化*表达式*在类或结构体声明的上下文中是可选的，在其他地方则是必需的。当类型可以从初始化*表达式*推断出来时，*类型*标注是可选的。这个表达式会在你第一次读取该属性的值时求值。如果你没有读取属性就覆盖了它的初始值，那么这个表达式会在第一次写入该属性之前求值。

`willSet` 和 `didSet` 观察器提供了一种在变量或属性的值被设置时进行观察（并做出适当响应）的方式。变量或属性首次初始化时不会调用这些观察器。它们只在初始化上下文之外设置值时才会被调用。

`willSet` 观察器在变量或属性的值被设置之前调用。新值作为常量传给 `willSet` 观察器，因此不能在 `willSet` 子句的实现中更改它。`didSet` 观察器在新值设置之后立即调用。与 `willSet` 观察器不同，变量或属性的旧值会传给 `didSet` 观察器，以备你仍然需要访问它。话虽如此，如果你在变量或属性自己的 `didSet` 观察器子句中给它赋值，那么你赋的新值会替换刚刚设置并传给 `willSet` 观察器的那个值。

*setter 名称*以及 `willSet` 和 `didSet` 子句中的外层括号是可选的。如果提供了 setter 名称，它们会被用作 `willSet` 和 `didSet` 观察器的形参名称。如果不提供 setter 名称，`willSet` 观察器的默认形参名是 `newValue`，`didSet` 观察器的默认形参名是 `oldValue`。

提供 `willSet` 子句时，`didSet` 子句是可选的。同样，提供 `didSet` 子句时，`willSet` 子句是可选的。

如果 `didSet` 观察器的主体引用了旧值，那么会在观察器之前调用 getter，以便提供旧值。否则，直接存储新值，不调用父类的 getter。下面的示例展示了一个由父类定义、被子类重写以添加观察器的计算属性。

```swift
class Superclass {
    private var xValue = 12
    var x: Int {
        get { print("Getter was called"); return xValue }
        set { print("Setter was called"); xValue = newValue }
    }
}

// This subclass doesn't refer to oldValue in its observer, so the
// superclass's getter is called only once to print the value.
class New: Superclass {
    override var x: Int {
        didSet { print("New value \(x)") }
    }
}
let new = New()
new.x = 100
// Prints "Setter was called".
// Prints "Getter was called".
// Prints "New value 100".

// This subclass refers to oldValue in its observer, so the superclass's
// getter is called once before the setter, and again to print the value.
class NewAndOld: Superclass {
    override var x: Int {
        didSet { print("Old value \(oldValue) - new value \(x)") }
    }
}
let newAndOld = NewAndOld()
newAndOld.x = 200
// Prints "Getter was called".
// Prints "Setter was called".
// Prints "Getter was called".
// Prints "Old value 12 - new value 200".
```

更多信息以及如何使用属性观察器的示例，参见 [属性观察器](properties.md#Property-Observers)。

### 类型变量属性 {#Type-Variable-Properties}

要声明类型变量属性，请用 `static` 声明修饰符标记该声明。类可以改用 `class` 声明修饰符标记类型计算属性，以允许子类重写父类的实现。类型属性在 [类型属性](properties.md#Type-Properties) 中讨论。

> 变量声明语法：
>
> *variable-declaration* → *variable-declaration-head* *pattern-initializer-list* \
> *variable-declaration* → *variable-declaration-head* *variable-name* *type-annotation* *code-block* \
> *variable-declaration* → *variable-declaration-head* *variable-name* *type-annotation* *getter-setter-block* \
> *variable-declaration* → *variable-declaration-head* *variable-name* *type-annotation* *getter-setter-keyword-block* \
> *variable-declaration* → *variable-declaration-head* *variable-name* *initializer* *willSet-didSet-block* \
> *variable-declaration* → *variable-declaration-head* *variable-name* *type-annotation* *initializer*_?_ *willSet-didSet-block*
>
> *variable-declaration-head* → *attributes*_?_ *declaration-modifiers*_?_ **`var`** \
> *variable-name* → *identifier*
>
> *getter-setter-block* → *code-block* \
> *getter-setter-block* → **`{`** *getter-clause* *setter-clause*_?_ **`}`** \
> *getter-setter-block* → **`{`** *setter-clause* *getter-clause* **`}`** \
> *getter-clause* → *attributes*_?_ *mutation-modifier*_?_ **`get`** *code-block* \
> *setter-clause* → *attributes*_?_ *mutation-modifier*_?_ **`set`** *setter-name*_?_ *code-block* \
> *setter-name* → **`(`** *identifier* **`)`**
>
> *getter-setter-keyword-block* → **`{`** *getter-keyword-clause* *setter-keyword-clause*_?_ **`}`** \
> *getter-setter-keyword-block* → **`{`** *setter-keyword-clause* *getter-keyword-clause* **`}`** \
> *getter-keyword-clause* → *attributes*_?_ *mutation-modifier*_?_ **`get`** \
> *setter-keyword-clause* → *attributes*_?_ *mutation-modifier*_?_ **`set`**
>
> *willSet-didSet-block* → **`{`** *willSet-clause* *didSet-clause*_?_ **`}`** \
> *willSet-didSet-block* → **`{`** *didSet-clause* *willSet-clause*_?_ **`}`** \
> *willSet-clause* → *attributes*_?_ **`willSet`** *setter-name*_?_ *code-block* \
> *didSet-clause* → *attributes*_?_ **`didSet`** *setter-name*_?_ *code-block*

## 类型别名声明 {#Type-Alias-Declaration}

*类型别名声明*向你的程序中引入已有类型的具名别名。类型别名声明使用 `typealias` 关键字声明，形式如下：

```swift
typealias <#name#> = <#existing type#>
```

声明类型别名之后，被起别名的 *name* 就可以在你的程序中任何地方代替*已有类型*使用。*已有类型*可以是具名类型或复合类型。类型别名不会创建新类型，它们只是让一个名称可以指代已有类型。

类型别名声明可以使用泛型参数为已有的泛型类型起名。类型别名可以为已有类型的部分或全部泛型参数提供具体类型。例如：

```swift
typealias StringDictionary<Value> = Dictionary<String, Value>

// The following dictionaries have the same type.
var dictionary1: StringDictionary<Int> = [:]
var dictionary2: Dictionary<String, Int> = [:]
```

用泛型参数声明类型别名时，这些参数上的约束必须与已有类型的泛型参数上的约束完全一致。例如：

```swift
typealias DictionaryOfInts<Key: Hashable> = Dictionary<Key, Int>
```

由于类型别名和已有类型可以互换使用，类型别名不能引入额外的泛型约束。

类型别名可以通过在声明中省略所有泛型参数，来沿用已有类型的泛型参数。例如，这里声明的 `Diccionario` 类型别名与 `Dictionary` 具有相同的泛型参数和约束。

```swift
typealias Diccionario = Dictionary
```

在协议声明内部，类型别名可以为频繁使用的类型提供一个更简短、更方便的名称。例如：

```swift
protocol Sequence {
    associatedtype Iterator: IteratorProtocol
    typealias Element = Iterator.Element
}

func sum<T: Sequence>(_ sequence: T) -> Int where T.Element == Int {
    // ...
}
```

如果没有这个类型别名，`sum` 函数就必须用 `T.Iterator.Element` 而不是 `T.Element` 来引用该关联类型。

另请参见 [协议关联类型声明](declarations.md#Protocol-Associated-Type-Declaration)。

> 类型别名声明语法：
>
> *typealias-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`typealias`** *typealias-name* *generic-parameter-clause*_?_ *typealias-assignment* \
> *typealias-name* → *identifier* \
> *typealias-assignment* → **`=`** *type*

## 函数声明 {#Function-Declaration}

*函数声明*向你的程序中引入函数或方法。在类、结构体、枚举或协议上下文中声明的函数称为*方法*。函数声明使用 `func` 关键字声明，形式如下：

```swift
func <#function name#>(<#parameters#>) -> <#return type#> {
   <#statements#>
}
```

如果函数的返回类型是 `Void`，可以省略返回类型，如下所示：

```swift
func <#function name#>(<#parameters#>) {
   <#statements#>
}
```

每个形参的类型都必须写出——类型不能被推断。如果在形参类型前面写 `inout`，该形参就可以在函数作用域内被修改。输入输出参数将在下文 [输入输出参数](declarations.md#In-Out-Parameters) 中详细讨论。

如果函数声明的*语句*只包含一个表达式，那么它被视为返回该表达式的值。只有当表达式的类型和函数的返回类型都不是 `Void`，也不是像 `Never` 这样没有任何成员的枚举时，才会考虑这种隐式返回语法。

函数可以使用元组类型作为返回类型来返回多个值。

函数定义可以出现在另一个函数声明内部。这种函数称为*嵌套函数*。

如果嵌套函数捕获的值保证永远不会逃逸（例如输入输出参数），或者该嵌套函数被作为非逃逸函数实参传入，那么它就是非逃逸的。否则，嵌套函数是逃逸函数。

关于嵌套函数的讨论，参见 [嵌套函数](functions.md#Nested-Functions)。

### 形参名称 {#Parameter-Names}

函数形参是一个逗号分隔的列表，每个形参都采用几种形式之一。函数调用中实参的顺序必须与函数声明中形参的顺序一致。形参列表中最简单的条目形式如下：

```swift
<#parameter name#>: <#parameter type#>
```

形参有一个名称，在函数体内部使用；还有一个实参标签，在调用函数或方法时使用。默认情况下，形参名称也用作实参标签。例如：

```swift
func f(x: Int, y: Int) -> Int { return x + y }
f(x: 1, y: 2) // both x and y are labeled
```

你可以用以下形式之一覆盖实参标签的默认行为：

```swift
<#argument label#> <#parameter name#>: <#parameter type#>
_ <#parameter name#>: <#parameter type#>
```

形参名称前面的名称会给形参一个显式的实参标签，它可以与形参名称不同。在函数或方法调用中，对应的实参必须使用给定的实参标签。

形参名称前面的下划线（`_`）会抑制实参标签。在函数或方法调用中，对应的实参必须不带标签。

```swift
func repeatGreeting(_ greeting: String, count n: Int) { /* Greet n times */ }
repeatGreeting("Hello, world!", count: 2) //  count is labeled, greeting is not
```

### 形参修饰符 {#Parameter-Modifiers}

*形参修饰符*改变实参传给函数的方式。

```swift
<#argument label#> <#parameter name#>: <#parameter modifier#> <#parameter type#>
```

要使用形参修饰符，请在实参的类型前面写 `inout`、`borrowing` 或 `consuming`。

```swift
func someFunction(a: inout A, b: consuming B, c: C) { ... }
```

#### 输入输出参数 {#In-Out-Parameters}

默认情况下，Swift 中的函数实参按值传递：在函数内部所做的任何更改在调用方都不可见。要改为输入输出参数，请应用 `inout` 形参修饰符。

```swift
func someFunction(a: inout Int) {
    a += 1
}
```

调用包含输入输出参数的函数时，输入输出实参前面必须加上与号（`&`），以表明该函数调用可以改变实参的值。

```swift
var x = 7
someFunction(a: &x)
print(x)  // Prints "8"
```

输入输出参数的传递过程如下：

1. 调用函数时，复制实参的值。
2. 在函数体中修改这个副本。
3. 函数返回时，把副本的值赋给原来的实参。

这种行为称为*拷入拷出*（copy-in copy-out）或*按值调用并返回结果*（call by value result）。例如，当计算属性或带观察器的属性作为输入输出参数传入时，它的 getter 会作为函数调用的一部分被调用，setter 会作为函数返回的一部分被调用。

作为一种优化，当实参是存储在内存中某个物理地址上的值时，函数体内部和外部会使用同一个内存位置。这种优化后的行为称为*按引用调用*（call by reference）；它满足拷入拷出模型的所有要求，同时消除了复制的开销。编写代码时请以拷入拷出模型为准，不要依赖按引用调用的优化，这样无论是否进行优化，代码都能正确运行。

在函数内部，不要访问作为输入输出实参传入的值，即使原值在当前作用域中可用。访问原值属于对该值的同时访问，违反了内存独占性。

```swift
var someValue: Int
func someFunction(a: inout Int) {
    a += someValue
}

// Error: This causes a runtime exclusivity violation
someFunction(a: &someValue)
```

出于同样的原因，你不能把同一个值传给多个输入输出参数。

```swift
var someValue: Int
func someFunction(a: inout Int, b: inout Int) {
    a += b
    b += 1
}

// Error: Cannot pass the same value to multiple in-out parameters
someFunction(a: &someValue, b: &someValue)
```

关于内存安全和内存独占性的更多信息，参见 [内存安全](memory-safety.md)。

捕获输入输出参数的闭包或嵌套函数必须是非逃逸的。如果你需要捕获输入输出参数但不修改它，请使用捕获列表以不可变方式显式捕获该参数。

```swift
func someFunction(a: inout Int) -> () -> Int {
    return { [a] in return a + 1 }
}
```

如果你需要捕获并修改输入输出参数，请使用显式的局部副本，例如在多线程代码中确保在函数返回之前所有修改都已完成。

```swift
func multithreadedFunction(queue: DispatchQueue, x: inout Int) {
    // Make a local copy and manually copy it back.
    var localX = x
    defer { x = localX }

    // Operate on localX asynchronously, then wait before returning.
    queue.async { someMutatingOperation(&localX) }
    queue.sync {}
}
```

关于输入输出参数的更多讨论和示例，参见 [输入输出参数](functions.md#In-Out-Parameters)。

#### 借用参数与消耗参数 {#Borrowing-and-Consuming-Parameters}

默认情况下，Swift 使用一组规则在函数调用之间自动管理对象的生命周期，并在需要时复制值。这些默认规则旨在尽可能降低大多数情况下的开销——如果你需要更精细的控制，可以应用 `borrowing` 或 `consuming` 形参修饰符。在这种情况下，使用 `copy` 显式标记复制操作。此外，不可复制类型的值必须以借用或消耗的方式传入。

无论你是否使用默认规则，Swift 都保证在所有情况下正确管理对象生命周期和所有权。这些形参修饰符只影响特定使用模式的相对效率，不影响正确性。

`borrowing` 修饰符表示函数不保留形参的值。在这种情况下，调用方保持对象的所有权，并负责对象的生命周期。当函数只是临时使用对象时，使用 `borrowing` 可以最大限度地降低开销。

```swift
// `isLessThan` does not keep either argument
func isLessThan(lhs: borrowing A, rhs: borrowing A) -> Bool {
    ...
}
```

如果函数需要保留形参的值——例如把它存储在全局变量中——你就用 `copy` 显式复制该值。

```swift
// As above, but this `isLessThan` also wants to record the smallest value
func isLessThan(lhs: borrowing A, rhs: borrowing A) -> Bool {
    if lhs < storedValue {
        storedValue = copy lhs
    } else if rhs < storedValue {
        storedValue = copy rhs
    }
    return lhs < rhs
}
```

相反，`consuming` 形参修饰符表示函数取得该值的所有权，并负责在函数返回之前存储或销毁它。

```swift
// `store` keeps its argument, so mark it `consuming`
func store(a: consuming A) {
    someGlobalVariable = a
}
```

当调用方在函数调用之后不再需要使用该对象时，使用 `consuming` 可以最大限度地降低开销。

```swift
// Usually, this is the last thing you do with a value
store(a: value)
```

如果你在函数调用之后继续使用某个可复制对象，编译器会在函数调用之前自动复制该对象。

```swift
// The compiler inserts an implicit copy here
store(a: someValue)  // This function consumes someValue
print(someValue)  // This uses the copy of someValue
```

与 `inout` 不同，`borrowing` 和 `consuming` 形参在调用函数时都不需要任何特殊记号：

```swift
func someFunction(a: borrowing A, b: consuming B) { ... }

someFunction(a: someA, b: someB)
```

显式使用 `borrowing` 或 `consuming`，表明你有意更严格地控制运行时所有权管理的开销。由于复制可能引起意料之外的运行时所有权操作，用这两种修饰符标记的形参不能被复制，除非使用显式的 `copy` 关键字：

```swift
func borrowingFunction1(a: borrowing A) {
    // Error: Cannot implicitly copy a
    // This assignment requires a copy because
    // `a` is only borrowed from the caller.
    someGlobalVariable = a
}

func borrowingFunction2(a: borrowing A) {
    // OK: Explicit copying works
    someGlobalVariable = copy a
}

func consumingFunction1(a: consuming A) {
    // Error: Cannot implicitly copy a
    // This assignment requires a copy because
    // of the following `print`
    someGlobalVariable = a
    print(a)
}

func consumingFunction2(a: consuming A) {
    // OK: Explicit copying works regardless
    someGlobalVariable = copy a
    print(a)
}

func consumingFunction3(a: consuming A) {
    // OK: No copy needed here because this is the last use
    someGlobalVariable = a
}
```

### 特殊种类的形参 {#Special-Kinds-of-Parameters}

形参可以被忽略，可以接受数量可变的值，也可以使用以下形式提供默认值：

```swift
_ : <#parameter type#>
<#parameter name#>: <#parameter type#>...
<#parameter name#>: <#parameter type#> = <#default argument value#>
```

下划线（`_`）形参会被显式忽略，在函数体内部无法访问。

基础类型名后面紧跟三个点（`...`）的形参被视为可变参数。紧跟在可变参数之后的形参必须带有实参标签。一个函数可以有多个可变参数。可变参数被视为一个数组，其元素类型就是该基础类型名所表示的类型。例如，可变参数 `Int...` 被视为 `[Int]`。使用可变参数的示例，参见 [可变参数](functions.md#Variadic-Parameters)。

在类型后面带有等号（`=`）和表达式的形参，被视为具有给定表达式的默认值。给定的表达式会在调用函数时求值。如果调用函数时省略该形参，则改用默认值。

```swift
func f(x: Int = 42) -> Int { return x }
f()       // Valid, uses default value
f(x: 7)   // Valid, uses the value provided
f(7)      // Invalid, missing argument label
```

### 特殊种类的方法 {#Special-Kinds-of-Methods}

枚举或结构体上修改 `self` 的方法必须用 `mutating` 声明修饰符标记。

重写父类方法的方法必须用 `override` 声明修饰符标记。没有 `override` 修饰符就重写方法，或者对不重写父类方法的方法使用 `override` 修饰符，都会产生编译时错误。

与类型（而不是类型的实例）关联的方法，在枚举和结构体中必须用 `static` 声明修饰符标记，在类中必须用 `static` 或 `class` 声明修饰符标记。用 `class` 声明修饰符标记的类类型方法可以由子类的实现重写；用 `class final` 或 `static` 标记的类类型方法不能被重写。

### 具有特殊名称的方法 {#Methods-with-Special-Names}

有几个具有特殊名称的方法可以为函数调用语法提供语法糖。如果某个类型定义了其中之一，该类型的实例就可以用于函数调用语法。这种函数调用被视为调用该实例上某个具有特殊名称的方法。

类、结构体或枚举类型可以通过定义 `dynamicallyCall(withArguments:)` 方法或 `dynamicallyCall(withKeywordArguments:)` 方法来支持函数调用语法，详见 [dynamicCallable](attributes.md#dynamicCallable)；也可以通过定义以函数方式调用的方法（call-as-function method）来支持，详见下文。如果该类型既定义了以函数方式调用的方法，又定义了 `dynamicCallable` 特性所用的方法之一，那么在两种方法都可使用的情况下，编译器优先选择以函数方式调用的方法。

以函数方式调用的方法的名称是 `callAsFunction()`，或者以 `callAsFunction(` 开头并加上带标签或不带标签的实参的其他名称——例如，`callAsFunction(_:_:)` 和 `callAsFunction(something:)` 也是有效的此类方法名称。

以下函数调用是等价的：

```swift
struct CallableStruct {
    var value: Int
    func callAsFunction(_ number: Int, scale: Int) {
        print(scale * (number + value))
    }
}
let callable = CallableStruct(value: 100)
callable(4, scale: 2)
callable.callAsFunction(4, scale: 2)
// Both function calls print 208.
```

以函数方式调用的方法与 `dynamicCallable` 特性的方法在两方面之间做了不同的权衡：把多少信息编码进类型系统，以及运行时能实现多少动态行为。声明以函数方式调用的方法时，你要指定实参的数量，以及每个实参的类型和标签。`dynamicCallable` 特性的方法只指定用于保存实参数组的类型。

定义以函数方式调用的方法，或者定义 `dynamicCallable` 特性的方法，都不能让你在函数调用表达式之外的任何上下文中把该类型的实例当作函数使用。例如：

```swift
let someFunction1: (Int, Int) -> Void = callable(_:scale:)  // Error
let someFunction2: (Int, Int) -> Void = callable.callAsFunction(_:scale:)
```

`subscript(dynamicMember:)` 下标为成员查找提供语法糖，详见 [dynamicMemberLookup](attributes.md#dynamicMemberLookup)。

### 抛出函数与方法 {#Throwing-Functions-and-Methods}

可以抛出错误的函数和方法必须用 `throws` 关键字标记。这类函数和方法称为*抛出函数*和*抛出方法*。它们的形式如下：

```swift
func <#function name#>(<#parameters#>) throws -> <#return type#> {
   <#statements#>
}
```

抛出特定错误类型的函数形式如下：

```swift
func <#function name#>(<#parameters#>) throws(<#error type#>) -> <#return type#> {
   <#statements#>
}
```

对抛出函数或方法的调用必须包裹在 `try` 或 `try!` 表达式中（也就是说，必须位于 `try` 或 `try!` 运算符的作用域内）。

函数的类型包括它能否抛出错误以及抛出何种类型的错误。这种子类型关系意味着，例如，你可以在需要抛出函数的地方使用不抛出错误的函数。关于抛出函数的类型的更多信息，参见 [函数类型](types.md#Function-Type)。使用具有显式类型的错误的示例，参见 [指定错误类型](error-handling.md#Specifying-the-Error-Type)。

你不能只根据函数能否抛出错误来重载函数。话虽如此，你可以根据函数的*形参*能否抛出错误来重载函数。

抛出方法不能重写不抛出错误的方法，抛出方法也不能满足针对不抛出错误方法的协议要求。话虽如此，不抛出错误的方法可以重写抛出方法，不抛出错误的方法也可以满足针对抛出方法的协议要求。

### 重新抛出函数与方法 {#Rethrowing-Functions-and-Methods}

函数或方法可以用 `rethrows` 关键字声明，表示只有当它的某个函数形参抛出错误时它才抛出错误。这类函数和方法称为*重新抛出函数*和*重新抛出方法*。重新抛出函数和方法必须至少有一个抛出函数的形参。

```swift
func someFunction(callback: () throws -> Void) rethrows {
    try callback()
}
```

重新抛出函数或方法只能在 `catch` 子句中包含 `throw` 语句。这让你可以在 `do`-`catch` 语句中调用抛出函数，并在 `catch` 子句中通过抛出另一个错误来处理错误。此外，`catch` 子句只能处理由该重新抛出函数的某个抛出形参抛出的错误。例如，下面的写法无效，因为该 `catch` 子句会处理 `alwaysThrows()` 抛出的错误。

```swift
func alwaysThrows() throws {
    throw SomeError.error
}
func someFunction(callback: () throws -> Void) rethrows {
    do {
        try callback()
        try alwaysThrows()  // Invalid, alwaysThrows() isn't a throwing parameter
    } catch {
        throw AnotherError.error
    }
}
```

抛出方法不能重写重新抛出方法，抛出方法也不能满足针对重新抛出方法的协议要求。话虽如此，重新抛出方法可以重写抛出方法，重新抛出方法也可以满足针对抛出方法的协议要求。

重新抛出的一种替代方案是在泛型代码中抛出特定错误类型。例如：

```swift
func someFunction<E: Error>(callback: () throws(E) -> Void) throws(E) {
    try callback()
}
```

这种传播错误的方式会保留关于错误的类型信息。不过，与把函数标记为 `rethrows` 不同，这种方式不会阻止该函数抛出相同类型的错误。

### 异步函数和方法 {#Asynchronous-Functions-and-Methods}

异步运行的函数和方法必须用 `async` 关键字标记。这类函数和方法称为*异步函数*和*异步方法*。它们的形式如下：

```swift
func <#function name#>(<#parameters#>) async -> <#return type#> {
   <#statements#>
}
```

对异步函数或方法的调用必须包裹在 `await` 表达式中——也就是说，它们必须位于 `await` 运算符的作用域内。

`async` 关键字是函数类型的一部分，同步函数是异步函数的子类型。因此，你可以在需要异步函数的地方使用同步函数。例如，你可以用同步方法重写异步方法，同步方法也可以满足要求异步方法的协议要求。

你可以根据函数是否异步来重载函数。在调用点，上下文决定使用哪个重载：在异步上下文中使用异步函数，在同步上下文中使用同步函数。

异步方法不能重写同步方法，异步方法也不能满足针对同步方法的协议要求。话虽如此，同步方法可以重写异步方法，同步方法也可以满足针对异步方法的协议要求。

### 永不返回的函数 {#Functions-that-Never-Return}

Swift 定义了 [`Never`][] 类型，用于表示函数或方法不会返回给调用方。返回类型为 `Never` 的函数和方法称为*永不返回的*（nonreturning）。永不返回的函数和方法要么导致不可恢复的错误，要么开始一段无限持续的工作。这意味着原本会在调用之后立即执行的代码永远不会执行。抛出函数和重新抛出函数即使永不返回，也可以把程序控制权转交给合适的 `catch` 块。

[`Never`]: https://developer.apple.com/documentation/swift/never

可以调用永不返回的函数或方法来结束 guard 语句的 `else` 子句，详见 [guard 语句](statements.md#Guard-Statement)。

你可以重写永不返回的方法，但新方法必须保持其返回类型和永不返回的行为。

> 函数声明语法：
>
> *function-declaration* → *function-head* *function-name* *generic-parameter-clause*_?_ *function-signature* *generic-where-clause*_?_ *function-body*_?_
>
> *function-head* → *attributes*_?_ *declaration-modifiers*_?_ **`func`** \
> *function-name* → *identifier* | *operator*
>
> *function-signature* → *parameter-clause* **`async`**_?_ *throws-clause*_?_ *function-result*_?_ \
> *function-signature* → *parameter-clause* **`async`**_?_ **`rethrows`** *function-result*_?_ \
> *function-result* → **`->`** *attributes*_?_ *type* \
> *function-body* → *code-block*
>
> *parameter-clause* → **`(`** **`)`** | **`(`** *parameter-list* **`,`**_?_ **`)`** \
> *parameter-list* → *parameter* | *parameter* **`,`** *parameter-list* \
> *parameter* → *external-parameter-name*_?_ *local-parameter-name* *parameter-type-annotation* *default-argument-clause*_?_ \
> *parameter* → *external-parameter-name*_?_ *local-parameter-name* *parameter-type-annotation* \
> *parameter* → *external-parameter-name*_?_ *local-parameter-name* *parameter-type-annotation* **`...`**
>
> *external-parameter-name* → *identifier* \
> *local-parameter-name* → *identifier* \
> *parameter-type-annotation* → **`:`** *attributes*_?_ *parameter-modifier*_?_ *type* \
> *parameter-modifier* → **`inout`** | **`borrowing`** | **`consuming`** *default-argument-clause* → **`=`** *expression*

## 枚举声明 {#Enumeration-Declaration}

*枚举声明*向你的程序中引入具名枚举类型。

枚举声明有两种基本形式，使用 `enum` 关键字声明。用任一形式声明的枚举，其主体包含零个或多个值——称为*枚举成员*——以及任意数量的声明，包括计算属性、实例方法、类型方法、构造器、类型别名，甚至其他枚举、结构体、类和 actor 声明。枚举声明不能包含析构器或协议声明。

枚举类型可以采纳任意数量的协议，但不能继承类、结构体或其他枚举。

与类和结构体不同，枚举类型没有隐式提供的默认构造器；所有构造器都必须显式声明。构造器可以代理给枚举中的其他构造器，但只有构造器把某个枚举成员赋给 `self` 之后，构造过程才算完成。

与结构体一样（但与类不同），枚举是值类型；枚举的实例在赋给变量或常量时，或者作为实参传给函数调用时，会被复制。关于值类型的信息，参见 [结构体和枚举是值类型](classes-and-structures.md#Structures-and-Enumerations-Are-Value-Types)。

你可以用扩展声明扩展枚举类型的行为，详见 [扩展声明](declarations.md#Extension-Declaration)。

### 成员可为任意类型的枚举 {#Enumerations-with-Cases-of-Any-Type}

以下形式声明包含任意类型枚举成员的枚举类型：

```swift
enum <#enumeration name#>: <#adopted protocols#> {
    case <#enumeration case 1#>
    case <#enumeration case 2#>(<#associated value types#>)
}
```

用这种形式声明的枚举在其他编程语言中有时称为*可辨识联合*（discriminated union）。

在这种形式中，每个成员块由 `case` 关键字后跟一个或多个枚举成员组成，成员之间用逗号分隔。每个成员的名称必须唯一。每个成员还可以指定它存储给定类型的值。这些类型在紧跟在成员名称之后的*关联值类型*元组中指定。

存储关联值的枚举成员可以当作函数使用，用来创建带有指定关联值的枚举实例。而且就像函数一样，你可以获取对枚举成员的引用，并在代码中稍后应用它。

```swift
enum Number {
    case integer(Int)
    case real(Double)
}
let f = Number.integer
// f is a function of type (Int) -> Number

// Apply f to create an array of Number instances with integer values
let evenInts: [Number] = [0, 2, 4, 6].map(f)
```

更多信息以及带关联值类型的成员示例，参见 [关联值](enumerations.md#Associated-Values)。

#### 带间接层的枚举 {#Enumerations-with-Indirection}

枚举可以具有递归结构，也就是说，它们可以有这样的成员：其关联值是枚举类型本身的实例。不过，枚举类型的实例具有值语义，这意味着它们在内存中有固定的布局。为了支持递归，编译器必须插入一层间接。

要为特定枚举成员启用间接，请用 `indirect` 声明修饰符标记它。间接成员必须有关联值。

```swift
enum Tree<T> {
    case empty
    indirect case node(value: T, left: Tree, right: Tree)
}
```

要为枚举中所有带关联值的成员启用间接，请用 `indirect` 修饰符标记整个枚举——当枚举包含许多每个都需要用 `indirect` 修饰符标记的成员时，这样做很方便。

用 `indirect` 修饰符标记的枚举可以混合包含带关联值的成员和不带关联值的成员。话虽如此，它不能包含任何同时也用 `indirect` 修饰符标记的成员。

### 成员为原始值类型的枚举 {#Enumerations-with-Cases-of-a-Raw-Value-Type}

以下形式声明包含同一基础类型枚举成员的枚举类型：

```swift
enum <#enumeration name#>: <#raw-value type#>, <#adopted protocols#> {
    case <#enumeration case 1#> = <#raw value 1#>
    case <#enumeration case 2#> = <#raw value 2#>
}
```

在这种形式中，每个成员块由 `case` 关键字后跟一个或多个枚举成员组成，成员之间用逗号分隔。与第一种形式中的成员不同，每个成员都有一个称为*原始值*（raw value）的底层值，且属于同一基础类型。这些值的类型由*原始值类型*指定，并且必须表示整数、浮点数、字符串或单个字符。具体来说，*原始值类型*必须遵循 `Equatable` 协议以及下列协议之一：整数字面量用 `ExpressibleByIntegerLiteral`，浮点字面量用 `ExpressibleByFloatLiteral`，包含任意数量字符的字符串字面量用 `ExpressibleByStringLiteral`，只包含单个字符的字符串字面量用 `ExpressibleByUnicodeScalarLiteral` 或 `ExpressibleByExtendedGraphemeClusterLiteral`。每个成员必须具有唯一的名称，并被赋予唯一的原始值。

如果原始值类型指定为 `Int`，并且你没有显式给成员赋值，那么它们会被隐式赋予 `0`、`1`、`2` 等值。每个未赋值的 `Int` 类型成员都会被隐式赋予一个原始值，该值从前一个成员的原始值自动递增得到。

```swift
enum ExampleEnum: Int {
    case a, b, c = 5, d
}
```

在上面的示例中，`ExampleEnum.a` 的原始值是 `0`，`ExampleEnum.b` 的值是 `1`。由于 `ExampleEnum.c` 的值被显式设为 `5`，`ExampleEnum.d` 的值会从 `5` 自动递增，因此是 `6`。

如果原始值类型指定为 `String`，并且你没有显式给成员赋值，那么每个未赋值的成员都会被隐式赋予一个字符串，其文本与该成员的名称相同。

```swift
enum GamePlayMode: String {
    case cooperative, individual, competitive
}
```

在上面的示例中，`GamePlayMode.cooperative` 的原始值是 `"cooperative"`，`GamePlayMode.individual` 的原始值是 `"individual"`，`GamePlayMode.competitive` 的原始值是 `"competitive"`。

具有原始值类型成员的枚举隐式遵循 Swift 标准库中定义的 `RawRepresentable` 协议。因此，它们有一个 `rawValue` 属性，以及一个签名为 `init?(rawValue: RawValue)` 的可失败构造器。你可以用 `rawValue` 属性访问枚举成员的原始值，例如 `ExampleEnum.b.rawValue`。你也可以通过调用枚举的可失败构造器，用原始值查找对应的成员（如果存在的话），例如 `ExampleEnum(rawValue: 5)`，它会返回一个可选成员。更多信息以及带原始值类型的成员示例，参见 [原始值](enumerations.md#Raw-Values)。

### 访问枚举成员 {#Accessing-Enumeration-Cases}

要引用枚举类型的成员，请使用点（`.`）语法，例如 `EnumerationType.enumerationCase`。当枚举类型可以从上下文中推断出来时，你可以省略它（点仍然必需），详见 [枚举语法](enumerations.md#Enumeration-Syntax) 和 [隐式成员表达式](expressions.md#Implicit-Member-Expression)。

要检查枚举成员的值，请使用 `switch` 语句，如 [使用 switch 语句匹配枚举值](enumerations.md#Matching-Enumeration-Values-with-a-Switch-Statement) 所示。枚举类型会与 `switch` 语句各成员块中的枚举成员模式进行模式匹配，详见 [枚举成员模式](patterns.md#Enumeration-Case-Pattern)。

> 枚举声明语法：
>
> *enum-declaration* → *attributes*_?_ *access-level-modifier*_?_ *union-style-enum* \
> *enum-declaration* → *attributes*_?_ *access-level-modifier*_?_ *raw-value-style-enum*
>
> *union-style-enum* → **`indirect`**_?_ **`enum`** *enum-name* *generic-parameter-clause*_?_ *type-inheritance-clause*_?_ *generic-where-clause*_?_ **`{`** *union-style-enum-members*_?_ **`}`** \
> *union-style-enum-members* → *union-style-enum-member* *union-style-enum-members*_?_ \
> *union-style-enum-member* → *declaration* | *union-style-enum-case-clause* | *compiler-control-statement* \
> *union-style-enum-case-clause* → *attributes*_?_ **`indirect`**_?_ **`case`** *union-style-enum-case-list* \
> *union-style-enum-case-list* → *union-style-enum-case* | *union-style-enum-case* **`,`** *union-style-enum-case-list* \
> *union-style-enum-case* → *enum-case-name* *tuple-type*_?_ \
> *enum-name* → *identifier* \
> *enum-case-name* → *identifier*
>
> *raw-value-style-enum* → **`enum`** *enum-name* *generic-parameter-clause*_?_ *type-inheritance-clause* *generic-where-clause*_?_ **`{`** *raw-value-style-enum-members* **`}`** \
> *raw-value-style-enum-members* → *raw-value-style-enum-member* *raw-value-style-enum-members*_?_ \
> *raw-value-style-enum-member* → *declaration* | *raw-value-style-enum-case-clause* | *compiler-control-statement* \
> *raw-value-style-enum-case-clause* → *attributes*_?_ **`case`** *raw-value-style-enum-case-list* \
> *raw-value-style-enum-case-list* → *raw-value-style-enum-case* | *raw-value-style-enum-case* **`,`** *raw-value-style-enum-case-list* \
> *raw-value-style-enum-case* → *enum-case-name* *raw-value-assignment*_?_ \
> *raw-value-assignment* → **`=`** *raw-value-literal* \
> *raw-value-literal* → *numeric-literal* | *static-string-literal* | *boolean-literal*

## 结构体声明 {#Structure-Declaration}

*结构体声明*向你的程序中引入具名结构体类型。结构体声明使用 `struct` 关键字声明，形式如下：

```swift
struct <#structure name#>: <#adopted protocols#> {
   <#declarations#>
}
```

结构体的主体包含零个或多个*声明*。这些*声明*可以包括存储属性和计算属性、类型属性、实例方法、类型方法、构造器、下标、类型别名，甚至其他结构体、类、actor 和枚举声明。结构体声明不能包含析构器或协议声明。关于包含各种声明的结构体的讨论和若干示例，参见 [结构体和类](classes-and-structures.md)。

结构体类型可以采纳任意数量的协议，但不能继承类、枚举或其他结构体。

有三种方式创建先前声明的结构体的实例：

- 调用结构体内声明的某个构造器，详见 [构造器](initialization.md#Initializers)。
- 如果没有声明任何构造器，调用结构体的成员逐一构造器，详见 [结构体类型的成员逐一构造器](initialization.md#Memberwise-Initializers-for-Structure-Types)。
- 如果没有声明任何构造器，并且结构体声明中的所有属性都已给定初始值，调用结构体的默认构造器，详见 [默认构造器](initialization.md#Default-Initializers)。

结构体所声明属性的初始化过程在 [构造过程](initialization.md) 中描述。

结构体实例的属性可以使用点（`.`）语法访问，详见 [访问属性](classes-and-structures.md#Accessing-Properties)。

结构体是值类型；结构体的实例在赋给变量或常量时，或者作为实参传给函数调用时，会被复制。关于值类型的信息，参见 [结构体和枚举是值类型](classes-and-structures.md#Structures-and-Enumerations-Are-Value-Types)。

你可以用扩展声明扩展结构体类型的行为，详见 [扩展声明](declarations.md#Extension-Declaration)。

> 结构体声明语法：
>
> *struct-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`struct`** *struct-name* *generic-parameter-clause*_?_ *type-inheritance-clause*_?_ *generic-where-clause*_?_ *struct-body* \
> *struct-name* → *identifier* \
> *struct-body* → **`{`** *struct-members*_?_ **`}`**
>
> *struct-members* → *struct-member* *struct-members*_?_ \
> *struct-member* → *declaration* | *compiler-control-statement*

## 类声明 {#Class-Declaration}

*类声明*向你的程序中引入具名类类型。类声明使用 `class` 关键字声明，形式如下：

```swift
class <#class name#>: <#superclass#>, <#adopted protocols#> {
   <#declarations#>
}
```

类的主体包含零个或多个*声明*。这些*声明*可以包括存储属性和计算属性、实例方法、类型方法、构造器、单个析构器、下标、类型别名，甚至其他类、结构体、actor 和枚举声明。类声明不能包含协议声明。关于包含各种声明的类的讨论和若干示例，参见 [结构体和类](classes-and-structures.md)。

类类型只能继承一个类，即它的*父类*（superclass），但可以采纳任意数量的协议。*父类*出现在*类名*和冒号之后的最前面，后面跟着任何*采纳的协议*。泛型类可以继承其他泛型类和非泛型类，但非泛型类只能继承其他非泛型类。在冒号后面写泛型父类的名称时，必须写出该泛型类的完整名称，包括其泛型参数子句。

如 [构造器声明](declarations.md#Initializer-Declaration) 所述，类可以有指定构造器和便利构造器。类的指定构造器必须初始化该类声明的所有属性，并且必须在调用其父类的任何指定构造器之前完成。

类可以重写其父类的属性、方法、下标和构造器。被重写的属性、方法、下标和指定构造器必须用 `override` 声明修饰符标记。

要要求子类实现父类的某个构造器，请用 `required` 声明修饰符标记父类的该构造器。子类对该构造器的实现也必须用 `required` 声明修饰符标记。

虽然*父类*中声明的属性和方法会被当前类继承，但*父类*中声明的指定构造器只有在子类满足 [自动构造器继承](initialization.md#Automatic-Initializer-Inheritance) 中所述条件时才会被继承。Swift 的类并不继承自某个通用基类。

有两种方式创建先前声明的类的实例：

- 调用类内声明的某个构造器，详见 [构造器](initialization.md#Initializers)。
- 如果没有声明任何构造器，并且类声明中的所有属性都已给定初始值，调用类的默认构造器，详见 [默认构造器](initialization.md#Default-Initializers)。

使用点（`.`）语法访问类实例的属性，详见 [访问属性](classes-and-structures.md#Accessing-Properties)。

类是引用类型；类的实例在赋给变量或常量时，或者作为实参传给函数调用时，是被引用而不是被复制。关于引用类型的信息，参见 [类是引用类型](classes-and-structures.md#Classes-Are-Reference-Types)。

你可以用扩展声明扩展类类型的行为，详见 [扩展声明](declarations.md#Extension-Declaration)。

> 类声明语法：
>
> *class-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`final`**_?_ **`class`** *class-name* *generic-parameter-clause*_?_ *type-inheritance-clause*_?_ *generic-where-clause*_?_ *class-body* \
> *class-declaration* → *attributes*_?_ **`final`** *access-level-modifier*_?_ **`class`** *class-name* *generic-parameter-clause*_?_ *type-inheritance-clause*_?_ *generic-where-clause*_?_ *class-body* \
> *class-name* → *identifier* \
> *class-body* → **`{`** *class-members*_?_ **`}`**
>
> *class-members* → *class-member* *class-members*_?_ \
> *class-member* → *declaration* | *compiler-control-statement*

## actor 声明 {#Actor-Declaration}

*actor 声明*向你的程序中引入具名 actor 类型。actor 声明使用 `actor` 关键字声明，形式如下：

```swift
actor <#actor name#>: <#adopted protocols#> {
    <#declarations#>
}
```

actor 的主体包含零个或多个*声明*。这些*声明*可以包括存储属性和计算属性、实例方法、类型方法、构造器、单个析构器、下标、类型别名，甚至其他类、结构体和枚举声明。关于包含各种声明的 actor 的讨论和若干示例，参见 [actor](concurrency.md#Actors)。

actor 类型可以采纳任意数量的协议，但不能继承类、枚举、结构体或其他 actor。不过，用 `@objc` 特性标记的 actor 隐式遵循 `NSObjectProtocol` 协议，并作为 `NSObject` 的子类型暴露给 Objective-C 运行时。

有两种方式创建先前声明的 actor 的实例：

- 调用 actor 内声明的某个构造器，详见 [构造器](initialization.md#Initializers)。
- 如果没有声明任何构造器，并且 actor 声明中的所有属性都已给定初始值，调用 actor 的默认构造器，详见 [默认构造器](initialization.md#Default-Initializers)。

默认情况下，actor 的成员隔离在该 actor 上。诸如方法体或属性的 getter 之类的代码会在该 actor 上执行。actor 内部的代码可以同步地与这些成员交互，因为该代码已经运行在同一个 actor 上；但 actor 外部的代码必须用 `await` 标记它们，以表明这段代码是在另一个 actor 上异步运行代码。key path 不能引用 actor 的隔离成员。actor 隔离的存储属性可以作为输入输出参数传给同步函数，但不能传给异步函数。

actor 也可以有非隔离成员，其声明用 `nonisolated` 关键字标记。非隔离成员的执行方式就像 actor 外部的代码：它不能与 actor 的任何隔离状态交互，调用方在使用它时也不用 `await` 标记。

actor 的成员只有在其为非隔离或异步时，才能用 `@objc` 特性标记。

actor 所声明属性的初始化过程在 [构造过程](initialization.md) 中描述。

actor 实例的属性可以使用点（`.`）语法访问，详见 [访问属性](classes-and-structures.md#Accessing-Properties)。

actor 是引用类型；actor 的实例在赋给变量或常量时，或者作为实参传给函数调用时，是被引用而不是被复制。关于引用类型的信息，参见 [类是引用类型](classes-and-structures.md#Classes-Are-Reference-Types)。

你可以用扩展声明扩展 actor 类型的行为，详见 [扩展声明](declarations.md#Extension-Declaration)。

> actor 声明语法：
>
> *actor-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`actor`** *actor-name* *generic-parameter-clause*_?_ *type-inheritance-clause*_?_ *generic-where-clause*_?_ *actor-body* \
> *actor-name* → *identifier* \
> *actor-body* → **`{`** *actor-members*_?_ **`}`**
>
> *actor-members* → *actor-member* *actor-members*_?_ \
> *actor-member* → *declaration* | *compiler-control-statement*

## 协议声明 {#Protocol-Declaration}

*协议声明*向你的程序中引入具名协议类型。协议声明使用 `protocol` 关键字声明，形式如下：

```swift
protocol <#protocol name#>: <#inherited protocols#> {
   <#protocol member declarations#>
}
```

协议声明可以出现在全局作用域，也可以嵌套在非泛型类型或非泛型函数内部。

协议的主体包含零个或多个*协议成员声明*，它们描述任何采纳该协议的类型都必须满足的一致性要求。具体来说，协议可以声明遵循类型必须实现某些属性、方法、构造器和下标。协议还可以声明特殊类型的类型别名，称为*关联类型*，用于指定协议各项声明之间的关系。协议声明不能包含类、结构体、枚举或其他协议声明。*协议成员声明*将在下文详细讨论。

协议类型可以继承任意数量的其他协议。当协议类型继承其他协议时，来自那些协议的要求会聚合在一起，任何继承当前协议的类型都必须遵循所有这些要求。关于如何使用协议继承的示例，参见 [协议继承](protocols.md#Protocol-Inheritance)。

> 注意：你也可以使用协议组合类型来聚合多个协议的一致性要求，详见 [协议组合类型](types.md#Protocol-Composition-Type) 和 [协议组合](protocols.md#Protocol-Composition)。

你可以通过在某个已声明类型的扩展声明中采纳协议，来为该类型添加协议一致性。在扩展中，你必须实现所采纳协议的所有要求。如果该类型已经实现了所有要求，你可以让扩展声明的主体为空。

默认情况下，遵循某个协议的类型必须实现该协议中声明的所有属性、方法和下标。话虽如此，你可以用 `optional` 声明修饰符标记这些协议成员声明，以指明遵循类型对它们的实现是可选的。`optional` 修饰符只能应用于用 `objc` 特性标记的成员，并且只能用于用 `objc` 特性标记的协议的成员。因此，只有类类型才能采纳并遵循包含可选成员要求的协议。关于如何使用 `optional` 声明修饰符的更多信息，以及如何访问可选协议成员的指导——例如，当你不确定某个遵循类型是否实现了它们时——参见 [可选协议要求](protocols.md#Optional-Protocol-Requirements)。

枚举成员可以满足针对类型成员的协议要求。具体来说，不带任何关联值的枚举成员满足针对类型为 `Self` 的只读类型变量的协议要求，而带关联值的枚举成员满足针对返回 `Self`、且其形参及实参标签与该成员关联值匹配的函数的协议要求。例如：

```swift
protocol SomeProtocol {
    static var someValue: Self { get }
    static func someFunction(x: Int) -> Self
}
enum MyEnum: SomeProtocol {
    case someValue
    case someFunction(x: Int)
}
```

要将协议的采纳限制为仅类类型，请在冒号后面的*继承的协议*列表中包含 `AnyObject` 协议。例如，下面的协议只能由类类型采纳：

```swift
protocol SomeProtocol: AnyObject {
    /* Protocol members go here. */
}
```

任何继承自带有 `AnyObject` 要求的协议的协议，同样只能由类类型采纳。

> 注意：如果协议用 `objc` 特性标记，`AnyObject` 要求会隐式应用于该协议；无需用 `AnyObject` 要求显式标记该协议。

你也可以在*继承的协议*列表中指定某个特定的类，从而把协议的采纳限制为该类的子类。例如，下面的协议只能由 `SomeBaseClass` 的子类采纳：

```swift
class SomeBaseClass { /* ... */ }
protocol SomeRefinedProtocol: SomeBaseClass {
    /* Protocol members go here. */
}
```

协议是具名类型，因此它们可以像其他具名类型一样出现在你代码中的所有相同位置，详见 [协议作为类型](protocols.md#Protocols-as-Types)。不过，你不能构造协议的实例，因为协议实际上并不为其规定的要求提供实现。

你可以用协议来声明类或结构体的委托对象应实现哪些方法，详见 [委托](protocols.md#Delegation)。

> 协议声明语法：
>
> *protocol-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`protocol`** *protocol-name* *type-inheritance-clause*_?_ *generic-where-clause*_?_ *protocol-body* \
> *protocol-name* → *identifier* \
> *protocol-body* → **`{`** *protocol-members*_?_ **`}`**
>
> *protocol-members* → *protocol-member* *protocol-members*_?_ \
> *protocol-member* → *protocol-member-declaration* | *compiler-control-statement*
>
> *protocol-member-declaration* → *protocol-property-declaration* \
> *protocol-member-declaration* → *protocol-method-declaration* \
> *protocol-member-declaration* → *protocol-initializer-declaration* \
> *protocol-member-declaration* → *protocol-subscript-declaration* \
> *protocol-member-declaration* → *protocol-associated-type-declaration* \
> *protocol-member-declaration* → *typealias-declaration*

### 协议属性声明 {#Protocol-Property-Declaration}

协议通过在协议声明的主体中包含*协议属性声明*，来声明遵循类型必须实现某个属性。协议属性声明采用变量声明的一种特殊形式：

```swift
var <#property name#>: <#type#> { get set }
```

与其他协议成员声明一样，这些属性声明只声明遵循该协议的类型对 getter 和 setter 的要求。因此，你不会在声明该协议的协议中直接实现 getter 或 setter。

getter 和 setter 要求可以由遵循类型以多种方式满足。如果属性声明同时包含 `get` 和 `set` 关键字，遵循类型可以用存储变量属性或可读可写的计算属性（即同时实现 getter 和 setter 的属性）来实现它。不过，该属性声明不能实现为常量属性或只读计算属性。如果属性声明只包含 `get` 关键字，则可以用任何种类的属性来实现。关于实现协议属性要求的遵循类型的示例，参见 [属性要求](protocols.md#Property-Requirements)。

要在协议声明中声明类型属性要求，请用 `static` 关键字标记该属性声明。遵循该协议的结构体和枚举用 `static` 关键字声明该属性，遵循该协议的类用 `static` 或 `class` 关键字声明该属性。为结构体、枚举或类添加协议一致性的扩展，使用与被扩展类型相同的关键字。为类型属性要求提供默认实现的扩展使用 `static` 关键字。

另请参见 [变量声明](declarations.md#Variable-Declaration)。

> 协议属性声明语法：
>
> *protocol-property-declaration* → *variable-declaration-head* *variable-name* *type-annotation* *getter-setter-keyword-block*

### 协议方法声明 {#Protocol-Method-Declaration}

协议通过在协议声明的主体中包含协议方法声明，来声明遵循类型必须实现某个方法。协议方法声明与函数声明形式相同，但有两个例外：它们不包含函数体，并且不能在函数声明中提供任何默认形参值。关于实现协议方法要求的遵循类型的示例，参见 [方法要求](protocols.md#Method-Requirements)。

要在协议声明中声明类方法或静态方法要求，请用 `static` 声明修饰符标记该方法声明。遵循该协议的结构体和枚举用 `static` 关键字声明该方法，遵循该协议的类用 `static` 或 `class` 关键字声明该方法。为结构体、枚举或类添加协议一致性的扩展，使用与被扩展类型相同的关键字。为类型方法要求提供默认实现的扩展使用 `static` 关键字。

另请参见 [函数声明](declarations.md#Function-Declaration)。

> 协议方法声明语法：
>
> *protocol-method-declaration* → *function-head* *function-name* *generic-parameter-clause*_?_ *function-signature* *generic-where-clause*_?_

### 协议构造器声明 {#Protocol-Initializer-Declaration}

协议通过在协议声明的主体中包含协议构造器声明，来声明遵循类型必须实现某个构造器。协议构造器声明与构造器声明形式相同，只是不包含构造器的主体。

遵循类型可以通过实现非可失败构造器或 `init!` 可失败构造器，来满足非可失败协议构造器要求。遵循类型可以通过实现任何种类的构造器，来满足可失败协议构造器要求。

当类实现某个构造器以满足协议的构造器要求时，如果该类尚未用 `final` 声明修饰符标记，则该构造器必须用 `required` 声明修饰符标记。

另请参见 [构造器声明](declarations.md#Initializer-Declaration)。

> 协议构造器声明语法：
>
> *protocol-initializer-declaration* → *initializer-head* *generic-parameter-clause*_?_ *parameter-clause* *throws-clause*_?_ *generic-where-clause*_?_ \
> *protocol-initializer-declaration* → *initializer-head* *generic-parameter-clause*_?_ *parameter-clause* **`rethrows`** *generic-where-clause*_?_

### 协议下标声明 {#Protocol-Subscript-Declaration}

协议通过在协议声明的主体中包含协议下标声明，来声明遵循类型必须实现某个下标。协议下标声明采用下标声明的一种特殊形式：

```swift
subscript (<#parameters#>) -> <#return type#> { get set }
```

下标声明只声明遵循该协议的类型对 getter 和 setter 实现的最低要求。如果下标声明同时包含 `get` 和 `set` 关键字，遵循类型必须同时实现 getter 子句和 setter 子句。如果下标声明只包含 `get` 关键字，遵循类型必须实现*至少*一个 getter 子句，并且可以选择实现 setter 子句。

要在协议声明中声明静态下标要求，请用 `static` 声明修饰符标记该下标声明。遵循该协议的结构体和枚举用 `static` 关键字声明该下标，遵循该协议的类用 `static` 或 `class` 关键字声明该下标。为结构体、枚举或类添加协议一致性的扩展，使用与被扩展类型相同的关键字。为静态下标要求提供默认实现的扩展使用 `static` 关键字。

另请参见 [下标声明](declarations.md#Subscript-Declaration)。

> 协议下标声明语法：
>
> *protocol-subscript-declaration* → *subscript-head* *subscript-result* *generic-where-clause*_?_ *getter-setter-keyword-block*

### 协议关联类型声明 {#Protocol-Associated-Type-Declaration}

协议使用 `associatedtype` 关键字声明关联类型。关联类型为协议声明中使用的某个类型提供一个别名。关联类型类似于泛型参数子句中的类型形参，但它们与声明它们的协议中的 `Self` 关联。在该上下文中，`Self` 指最终遵循该协议的类型。更多信息和示例，参见 [关联类型](generics.md#Associated-Types)。

你可以在协议声明中使用泛型 `where` 子句，为从另一个协议继承的关联类型添加约束，而无需重新声明这些关联类型。例如，下面 `SubProtocol` 的两个声明是等价的：

```swift
protocol SomeProtocol {
    associatedtype SomeType
}

protocol SubProtocolA: SomeProtocol {
    // This syntax produces a warning.
    associatedtype SomeType: Equatable
}

// This syntax is preferred.
protocol SubProtocolB: SomeProtocol where SomeType: Equatable { }
```

另请参见 [类型别名声明](declarations.md#Type-Alias-Declaration)。

> 协议关联类型声明语法：
>
> *protocol-associated-type-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`associatedtype`** *typealias-name* *type-inheritance-clause*_?_ *typealias-assignment*_?_ *generic-where-clause*_?_

## 构造器声明 {#Initializer-Declaration}

*构造器声明*向你的程序中引入类、结构体或枚举的构造器。构造器声明使用 `init` 关键字声明，有两种基本形式。

结构体、枚举和类类型可以有任意数量的构造器，但类构造器的规则和相关行为有所不同。与结构体和枚举不同，类有两种构造器：指定构造器和便利构造器，详见 [构造过程](initialization.md)。

以下形式声明结构体、枚举的构造器以及类的指定构造器：

```swift
init(<#parameters#>) {
   <#statements#>
}
```

类的指定构造器直接初始化该类的所有属性。它不能调用同一个类的任何其他构造器；如果该类有父类，它必须调用父类的某个指定构造器。如果类从父类继承了任何属性，那么必须在当前类中设置或修改这些属性之前，先调用父类的某个指定构造器。

指定构造器只能在类声明的上下文中声明，因此不能使用扩展声明把它添加到类中。

结构体和枚举中的构造器可以调用其他已声明的构造器，来代理部分或全部构造过程。

要为类声明便利构造器，请用 `convenience` 声明修饰符标记该构造器声明。

```swift
convenience init(<#parameters#>) {
   <#statements#>
}
```

便利构造器可以把构造过程代理给另一个便利构造器，或者代理给该类的某个指定构造器。话虽如此，构造过程必须以调用某个指定构造器结束，由它最终初始化类的属性。便利构造器不能调用父类的构造器。

你可以用 `required` 声明修饰符标记指定构造器和便利构造器，以要求每个子类都实现该构造器。子类对该构造器的实现也必须用 `required` 声明修饰符标记。

默认情况下，父类中声明的构造器不会被子类继承。话虽如此，如果子类用默认值初始化了它的所有存储属性，并且没有定义自己的任何构造器，那么它会继承父类的所有构造器。如果子类重写了父类的所有指定构造器，它就会继承父类的便利构造器。

与方法、属性和下标一样，你需要用 `override` 声明修饰符标记被重写的指定构造器。

> 注意：如果你用 `required` 声明修饰符标记某个构造器，那么在子类中重写这个必要构造器时，不要再用 `override` 修饰符标记该构造器。

与函数和方法一样，构造器可以抛出或重新抛出错误。同样与函数和方法一样，你在构造器的形参后面使用 `throws` 或 `rethrows` 关键字来表明相应的行为。同样，构造器也可以是异步的，你用 `async` 关键字来表明这一点。

要查看各种类型声明中构造器的示例，参见 [构造过程](initialization.md)。

### 可失败构造器 {#Failable-Initializers}

*可失败构造器*是一种构造器，它产生其声明所在类型的可选实例或隐式解包可选实例。因此，可失败构造器可以返回 `nil` 来表示构造失败。

要声明产生可选实例的可失败构造器，请在构造器声明中的 `init` 关键字后面加上问号（`init?`）。要声明产生隐式解包可选实例的可失败构造器，请改为加上感叹号（`init!`）。下面的示例展示了一个 `init?` 可失败构造器，它产生结构体的可选实例。

```swift
struct SomeStruct {
    let property: String
    // produces an optional instance of 'SomeStruct'
    init?(input: String) {
        if input.isEmpty {
            // discard 'self' and return 'nil'
            return nil
        }
        property = input
    }
}
```

调用 `init?` 可失败构造器的方式与调用非可失败构造器相同，只是你必须处理结果的可选性。

```swift
if let actualInstance = SomeStruct(input: "Hello") {
    // do something with the instance of 'SomeStruct'
} else {
    // initialization of 'SomeStruct' failed and the initializer returned 'nil'
}
```

可失败构造器可以在构造器主体实现中的任何位置返回 `nil`。

可失败构造器可以代理给任何种类的构造器。非可失败构造器可以代理给另一个非可失败构造器，或者代理给 `init!` 可失败构造器。非可失败构造器可以通过强制解包父类构造器的结果，来代理给 `init?` 可失败构造器——例如，写成 `super.init()!`。

构造失败会通过构造器代理传播。具体来说，如果可失败构造器代理给一个失败并返回 `nil` 的构造器，那么进行代理的构造器也会失败并隐式返回 `nil`。如果非可失败构造器代理给一个失败并返回 `nil` 的 `init!` 可失败构造器，就会引发运行时错误（就像你用 `!` 运算符解包一个值为 `nil` 的可选值一样）。

可失败指定构造器可以在子类中被任何种类的指定构造器重写。非可失败指定构造器在子类中只能被非可失败指定构造器重写。

更多信息以及可失败构造器的示例，参见 [可失败构造器](initialization.md#Failable-Initializers)。

> 构造器声明语法：
>
> *initializer-declaration* → *initializer-head* *generic-parameter-clause*_?_ *parameter-clause* **`async`**_?_ *throws-clause*_?_ *generic-where-clause*_?_ *initializer-body* \
> *initializer-declaration* → *initializer-head* *generic-parameter-clause*_?_ *parameter-clause* **`async`**_?_ **`rethrows`** *generic-where-clause*_?_ *initializer-body* \
> *initializer-head* → *attributes*_?_ *declaration-modifiers*_?_ **`init`** \
> *initializer-head* → *attributes*_?_ *declaration-modifiers*_?_ **`init`** **`?`** \
> *initializer-head* → *attributes*_?_ *declaration-modifiers*_?_ **`init`** **`!`** \
> *initializer-body* → *code-block*

## 析构器声明 {#Deinitializer-Declaration}

*析构器声明*为类类型声明析构器。析构器不接受任何形参，形式如下：

```swift
deinit {
   <#statements#>
}
```

当不再有对类对象的任何引用时，析构器会在该对象被释放之前自动调用。析构器只能在类声明的主体中声明——不能写在类的扩展中——并且每个类最多只能有一个。

子类继承父类的析构器，父类的析构器会在子类对象被释放之前隐式调用。只有在继承链上的所有析构器都执行完毕之后，子类对象才会被释放。

不能直接调用析构器。

关于如何在类声明中使用析构器的示例，参见 [析构过程](deinitialization.md)。

> 析构器声明语法：
>
> *deinitializer-declaration* → *attributes*_?_ **`deinit`** *code-block*

## 扩展声明 {#Extension-Declaration}

*扩展声明*允许你扩展已有类型的行为。扩展声明使用 `extension` 关键字声明，形式如下：

```swift
extension <#type name#> where <#requirements#> {
   <#declarations#>
}
```

扩展声明的主体包含零个或多个*声明*。这些*声明*可以包括计算属性、计算类型属性、实例方法、类型方法、构造器、下标声明，甚至类、结构体和枚举声明。扩展声明不能包含析构器或协议声明、存储属性、属性观察器或其他扩展声明。协议扩展中的声明不能标记为 `final`。关于包含各种声明的扩展的讨论和若干示例，参见 [扩展](extensions.md)。

如果*类型名称*是类、结构体或枚举类型，该扩展就扩展该类型。如果*类型名称*是协议类型，该扩展就扩展遵循该协议的所有类型。

扩展泛型类型或带关联类型的协议的扩展声明可以包含*要求*。如果被扩展类型的实例，或者遵循被扩展协议的类型的实例满足这些*要求*，该实例就会获得声明中指定的行为。

扩展声明可以包含构造器声明。话虽如此，如果你要扩展的类型定义在另一个模块中，构造器声明必须代理给该模块中已经定义的构造器，以确保该类型的成员被正确初始化。

已有类型的属性、方法和构造器不能在该类型的扩展中被重写。

扩展声明可以通过指定*采纳的协议*，为已有的类、结构体或枚举类型添加协议一致性：

```swift
extension <#type name#>: <#adopted protocols#> where <#requirements#> {
   <#declarations#>
}
```

扩展声明不能为已有的类添加类继承，因此在*类型名称*和冒号之后只能指定协议列表。

### 条件一致性 {#Conditional-Conformance}

你可以扩展泛型类型，使其有条件地遵循某个协议，这样该类型的实例只有在满足特定要求时才遵循该协议。你通过在扩展声明中包含*要求*，来添加对协议的条件一致性。

#### 在某些泛型上下文中不使用被重写的要求 {#Overridden-Requirements-Arent-Used-in-Some-Generic-Contexts}

在某些泛型上下文中，通过条件一致性从协议获得行为的类型并不总是使用该协议要求的特化实现。为了说明这种行为，下面的示例定义了两个协议，以及一个有条件地遵循这两个协议的泛型类型。

```swift
protocol Loggable {
    func log()
}
extension Loggable {
    func log() {
        print(self)
    }
}

protocol TitledLoggable: Loggable {
    static var logTitle: String { get }
}
extension TitledLoggable {
    func log() {
        print("\(Self.logTitle): \(self)")
    }
}

struct Pair<T>: CustomStringConvertible {
    let first: T
    let second: T
    var description: String {
        return "(\(first), \(second))"
    }
}

extension Pair: Loggable where T: Loggable { }
extension Pair: TitledLoggable where T: TitledLoggable {
    static var logTitle: String {
        return "Pair of '\(T.logTitle)'"
    }
}

extension String: TitledLoggable {
    static var logTitle: String {
        return "String"
    }
}
```

当 `Pair` 的泛型类型分别遵循 `Loggable` 或 `TitledLoggable` 时，`Pair` 结构体便遵循 `Loggable` 或 `TitledLoggable`。在下面的示例中，`oneAndTwo` 是 `Pair<String>` 的实例，由于 `String` 遵循 `TitledLoggable`，它遵循 `TitledLoggable`。直接对 `oneAndTwo` 调用 `log()` 方法时，使用的是包含标题字符串的特化版本。

```swift
let oneAndTwo = Pair(first: "one", second: "two")
oneAndTwo.log()
// Prints "Pair of 'String': (one, two)".
```

然而，当 `oneAndTwo` 用在泛型上下文中，或者作为 `Loggable` 协议的实例使用时，不会使用特化版本。Swift 仅根据 `Pair` 遵循 `Loggable` 所需的最低要求来选择调用 `log()` 的哪个实现。因此，这里改用 `Loggable` 协议提供的默认实现。

```swift
func doSomething<T: Loggable>(with x: T) {
    x.log()
}
doSomething(with: oneAndTwo)
// Prints "(one, two)".
```

当对传给 `doSomething(_:)` 的实例调用 `log()` 时，日志字符串中省略了自定义的标题。

### 协议一致性不得冗余 {#Protocol-Conformance-Must-Not-Be-Redundant}

具体类型只能遵循某个特定协议一次。Swift 会把冗余的协议一致性标记为错误。你可能在两种情况下遇到这类错误。第一种情况是你多次显式遵循同一个协议，但每次的要求不同。第二种情况是你多次隐式继承同一个协议。下文各节将讨论这些情况。

#### 解决显式冗余 {#Resolving-Explicit-Redundancy}

同一个具体类型上的多个扩展不能添加对同一个协议的一致性，即使这些扩展的要求互斥。下面的示例演示了这一限制。两个扩展声明都试图为 `Serializable` 协议添加条件一致性，一个针对元素为 `Int` 的数组，另一个针对元素为 `String` 的数组。

```swift
protocol Serializable {
    func serialize() -> Any
}

extension Array: Serializable where Element == Int {
    func serialize() -> Any {
        // implementation
    }
}
extension Array: Serializable where Element == String {
    func serialize() -> Any {
        // implementation
    }
}
// Error: Redundant conformance of 'Array<Element>' to protocol 'Serializable'.
```

如果你需要基于多个具体类型添加条件一致性，请创建一个新协议，让每个类型都能遵循它，并在声明条件一致性时把该协议用作要求。

```swift
protocol SerializableInArray { }
extension Int: SerializableInArray { }
extension String: SerializableInArray { }

extension Array: Serializable where Element: SerializableInArray {
    func serialize() -> Any {
        // implementation
    }
}
```

#### 解决隐式冗余 {#Resolving-Implicit-Redundancy}

当具体类型有条件地遵循某个协议时，该类型会隐式遵循具有相同要求的任何父协议。

如果你需要让某个类型有条件地遵循两个继承自同一父协议的协议，请显式声明对父协议的一致性。这样可以避免以不同的要求隐式遵循父协议两次。

下面的示例显式声明了 `Array` 对 `Loggable` 的条件一致性，以避免在同时声明对 `TitledLoggable` 和新 `MarkedLoggable` 协议的条件一致性时发生冲突。

```swift
protocol MarkedLoggable: Loggable {
    func markAndLog()
}

extension MarkedLoggable {
    func markAndLog() {
        print("----------")
        log()
    }
}

extension Array: Loggable where Element: Loggable { }
extension Array: TitledLoggable where Element: TitledLoggable {
    static var logTitle: String {
        return "Array of '\(Element.logTitle)'"
    }
}
extension Array: MarkedLoggable where Element: MarkedLoggable { }
```

如果没有这个扩展来显式声明对 `Loggable` 的条件一致性，其他 `Array` 扩展会隐式创建这些声明，从而导致错误：

```swift
extension Array: Loggable where Element: TitledLoggable { }
extension Array: Loggable where Element: MarkedLoggable { }
// Error: Redundant conformance of 'Array<Element>' to protocol 'Loggable'.
```

> 扩展声明语法：
>
> *extension-declaration* → *attributes*_?_ *access-level-modifier*_?_ **`extension`** *type-identifier* *type-inheritance-clause*_?_ *generic-where-clause*_?_ *extension-body* \
> *extension-body* → **`{`** *extension-members*_?_ **`}`**
>
> *extension-members* → *extension-member* *extension-members*_?_ \
> *extension-member* → *declaration* | *compiler-control-statement*

## 下标声明 {#Subscript-Declaration}

*下标*声明允许你为特定类型的对象添加下标支持，通常用于为访问集合、列表或序列中的元素提供方便的语法。下标声明使用 `subscript` 关键字声明，形式如下：

```swift
subscript (<#parameters#>) -> <#return type#> {
   get {
      <#statements#>
   }
   set(<#setter name#>) {
      <#statements#>
   }
}
```

下标声明只能出现在类、结构体、枚举、扩展或协议声明的上下文中。

*形参*指定下标表达式中用于访问相应类型元素的一个或多个索引（例如表达式 `object[i]` 中的 `i`）。虽然用于访问元素的索引可以是任何类型，但每个形参都必须包含类型标注，以指定每个索引的类型。*返回类型*指定被访问元素的类型。

与计算属性一样，下标声明支持读取和写入被访问元素的值。getter 用于读取值，setter 用于写入值。setter 子句是可选的，当只需要 getter 时，你可以同时省略这两个子句，直接返回所请求的值。话虽如此，如果你提供了 setter 子句，就必须同时提供 getter 子句。

*setter 名称*及其外层括号是可选的。如果你提供了 setter 名称，它会被用作 setter 形参的名称。如果不提供 setter 名称，setter 的默认形参名是 `newValue`。setter 形参的类型与*返回类型*相同。

你可以在声明下标的类型中重载下标声明，只要*形参*或*返回类型*与你正在重载的下标不同。你也可以重写从父类继承的下标声明。这样做时，必须用 `override` 声明修饰符标记被重写的下标声明。

下标形参遵循与函数形参相同的规则，但有两个例外。默认情况下，与函数、方法和构造器不同，用于下标的形参没有实参标签。不过，你可以使用与函数、方法和构造器相同的语法提供显式的实参标签。此外，下标不能有输入输出参数。下标形参可以有默认值，使用 [特殊种类的形参](declarations.md#Special-Kinds-of-Parameters) 中描述的语法。

你也可以在协议声明的上下文中声明下标，详见 [协议下标声明](declarations.md#Protocol-Subscript-Declaration)。

关于下标的更多信息以及下标声明的示例，参见 [下标](subscripts.md)。

### 类型下标声明 {#Type-Subscript-Declarations}

要声明由类型（而不是类型的实例）暴露的下标，请用 `static` 声明修饰符标记该下标声明。类可以改用 `class` 声明修饰符标记类型计算属性，以允许子类重写父类的实现。在类声明中，`static` 关键字的效果等同于同时用 `class` 和 `final` 声明修饰符标记该声明。

> 下标声明语法：
>
> *subscript-declaration* → *subscript-head* *subscript-result* *generic-where-clause*_?_ *code-block* \
> *subscript-declaration* → *subscript-head* *subscript-result* *generic-where-clause*_?_ *getter-setter-block* \
> *subscript-declaration* → *subscript-head* *subscript-result* *generic-where-clause*_?_ *getter-setter-keyword-block* \
> *subscript-head* → *attributes*_?_ *declaration-modifiers*_?_ **`subscript`** *generic-parameter-clause*_?_ *parameter-clause* \
> *subscript-result* → **`->`** *attributes*_?_ *type*

## 宏声明 {#Macro-Declaration}

*宏声明*引入一个新宏。它以 `macro` 关键字开头，形式如下：

```swift
macro <#name#> = <#macro implementation#>
```

*宏实现*是另一个宏，它指出执行此宏展开的代码所在的位置。执行宏展开的代码是一个独立的 Swift 程序，它使用 [SwiftSyntax][] 模块与 Swift 代码交互。调用 Swift 标准库中的 `externalMacro(module:type:)` 宏，传入包含该宏实现的类型名称，以及包含该类型的模块名称。

[SwiftSyntax]: https://github.com/swiftlang/swift-syntax

宏可以重载，遵循与函数相同的模型。宏声明只能出现在文件作用域。

关于 Swift 中宏的概述，参见 [宏](macros.md)。

> 宏声明语法：
>
> *macro-declaration* → *macro-head* *identifier* *generic-parameter-clause*_?_ *macro-signature* *macro-definition*_?_ *generic-where-clause* \
> *macro-head* → *attributes*_?_ *declaration-modifiers*_?_ **`macro`** \
> *macro-signature* → *parameter-clause* *macro-function-signature-result*_?_ \
> *macro-function-signature-result* → **`->`** *type* \
> *macro-definition* → **`=`** *expression*

## 运算符声明 {#Operator-Declaration}

*运算符声明*向你的程序中引入新的中缀、前缀或后缀运算符，使用 `operator` 关键字声明。

你可以声明三种不同书写位置的运算符：中缀、前缀和后缀。运算符的*书写位置*（fixity）决定运算符相对于其操作数的位置。

运算符声明有三种基本形式，每种书写位置对应一种。运算符的书写位置通过在 `operator` 关键字之前用 `infix`、`prefix` 或 `postfix` 声明修饰符标记运算符声明来指定。在每种形式中，运算符名称只能包含 [运算符](lexical-structure.md#Operators) 中定义的运算符字符。

以下形式声明新的中缀运算符：

```swift
infix operator <#operator name#>: <#precedence group#>
```

*中缀运算符*是写在两个操作数之间的二元运算符，例如表达式 `1 + 2` 中熟悉的加法运算符（`+`）。

中缀运算符可以选择指定优先级组。如果省略运算符的优先级组，Swift 会使用默认优先级组 `DefaultPrecedence`，它的优先级仅高于 `TernaryPrecedence`。更多信息，参见 [优先级组声明](declarations.md#Precedence-Group-Declaration)。

以下形式声明新的前缀运算符：

```swift
prefix operator <#operator name#>
```

*前缀运算符*是紧写在操作数之前的一元运算符，例如表达式 `!a` 中的前缀逻辑非运算符（`!`）。

前缀运算符声明不指定优先级。前缀运算符是非结合的。

以下形式声明新的后缀运算符：

```swift
postfix operator <#operator name#>
```

*后缀运算符*是紧写在操作数之后的一元运算符，例如表达式 `a!` 中的后缀强制解包运算符（`!`）。

与前缀运算符一样，后缀运算符声明不指定优先级。后缀运算符是非结合的。

声明新运算符之后，你通过声明一个与运算符同名的静态方法来实现它。该静态方法是某个类型的成员，而该类型的值正是运算符接受的实参之一——例如，把 `Double` 乘以 `Int` 的运算符，实现为 `Double` 或 `Int` 结构体上的静态方法。如果你要实现前缀或后缀运算符，还必须用相应的 `prefix` 或 `postfix` 声明修饰符标记该方法声明。要查看如何创建并实现新运算符的示例，参见 [自定义运算符](advanced-operators.md#Custom-Operators)。

> 运算符声明语法：
>
> *operator-declaration* → *prefix-operator-declaration* | *postfix-operator-declaration* | *infix-operator-declaration*
>
> *prefix-operator-declaration* → **`prefix`** **`operator`** *operator* \
> *postfix-operator-declaration* → **`postfix`** **`operator`** *operator* \
> *infix-operator-declaration* → **`infix`** **`operator`** *operator* *infix-operator-group*_?_
>
> *infix-operator-group* → **`:`** *precedence-group-name*

## 优先级组声明 {#Precedence-Group-Declaration}

*优先级组声明*向你的程序中引入新的中缀运算符优先级分组。在没有分组括号的情况下，运算符的优先级决定该运算符与其操作数结合的紧密程度。

优先级组声明的形式如下：

```swift
precedencegroup <#precedence group name#> {
    higherThan: <#lower group names#>
    lowerThan: <#higher group names#>
    associativity: <#associativity#>
    assignment: <#assignment#>
}
```

*较低优先级组名称*和*较高优先级组名称*列表指定新优先级组与现有优先级组的关系。`lowerThan` 优先级组属性只能用于引用当前模块之外声明的优先级组。当两个运算符争夺操作数时，例如在表达式 `2 + 3 * 5` 中，相对优先级较高的运算符与其操作数结合得更紧密。

> 注意：使用*较低优先级组名称*和*较高优先级组名称*相互关联的优先级组必须构成单一的关联层级，但它们*不必*形成线性层级。这意味着可以有相对优先级未定义的优先级组。来自这些优先级组的运算符不能在没有分组括号的情况下相邻使用。

Swift 为 Swift 标准库提供的运算符定义了许多优先级组。例如，加法（`+`）和减法（`-`）运算符属于 `AdditionPrecedence` 组，乘法（`*`）和除法（`/`）运算符属于 `MultiplicationPrecedence` 组。Swift 标准库提供的优先级组的完整列表，参见 [Operator Declarations](https://developer.apple.com/documentation/swift/operator_declarations)。

运算符的*结合性*决定在没有分组括号的情况下，具有相同优先级的一系列运算符如何组合在一起。你通过写上下文相关的关键字 `left`、`right` 或 `none` 之一来指定运算符的结合性——如果省略结合性，默认值是 `none`。左结合运算符从左到右组合。例如，减法运算符（`-`）是左结合的，因此表达式 `4 - 5 - 6` 组合为 `(4 - 5) - 6`，求值为 `-7`。右结合运算符从右到左组合，而结合性指定为 `none` 的运算符完全不结合。相同优先级的非结合运算符不能彼此相邻出现。例如，`<` 运算符的结合性是 `none`，这意味着 `1 < 2 < 3` 不是有效的表达式。

优先级组的*赋值*（assignment）指定运算符在包含可选链的运算中使用时的优先级。当设为 `true` 时，相应优先级组中的运算符在可选链期间使用与 Swift 标准库中的赋值运算符相同的分组规则。否则，当设为 `false` 或省略时，该优先级组中的运算符遵循与不执行赋值的运算符相同的可选链规则。

> 优先级组声明语法：
>
> *precedence-group-declaration* → **`precedencegroup`** *precedence-group-name* **`{`** *precedence-group-attributes*_?_ **`}`**
>
> *precedence-group-attributes* → *precedence-group-attribute* *precedence-group-attributes*_?_ \
> *precedence-group-attribute* → *precedence-group-relation* \
> *precedence-group-attribute* → *precedence-group-assignment* \
> *precedence-group-attribute* → *precedence-group-associativity*
>
> *precedence-group-relation* → **`higherThan`** **`:`** *precedence-group-names* \
> *precedence-group-relation* → **`lowerThan`** **`:`** *precedence-group-names*
>
> *precedence-group-assignment* → **`assignment`** **`:`** *boolean-literal*
>
> *precedence-group-associativity* → **`associativity`** **`:`** **`left`** \
> *precedence-group-associativity* → **`associativity`** **`:`** **`right`** \
> *precedence-group-associativity* → **`associativity`** **`:`** **`none`**
>
> *precedence-group-names* → *precedence-group-name* | *precedence-group-name* **`,`** *precedence-group-names* \
> *precedence-group-name* → *identifier*

## 声明修饰符 {#Declaration-Modifiers}

*声明修饰符*是修饰声明行为或含义的关键字或上下文相关关键字。你通过在声明的特性（如果有）和引入声明的关键字之间写出相应的关键字或上下文相关关键字，来指定声明修饰符。

- **`class`**：将此修饰符应用于类的成员，表示该成员是类本身的成员，而不是类实例的成员。父类中带有此修饰符且没有 `final` 修饰符的成员可以被子类重写。

- **`dynamic`**：将此修饰符应用于可由 Objective-C 表示的类的任何成员。当你用 `dynamic` 修饰符标记成员声明时，对该成员的访问总是使用 Objective-C 运行时进行动态派发。编译器从不会内联或去虚拟化对该成员的访问。

由于用 `dynamic` 修饰符标记的声明使用 Objective-C 运行时派发，它们必须用 `objc` 特性标记。

- **`final`**：将此修饰符应用于类，或应用于类的属性、方法或下标成员。应用于类时，表示该类不能被子类化。应用于类的属性、方法或下标时，表示该类成员不能在任何子类中被重写。关于如何使用 `final` 修饰符的示例，参见 [阻止重写](inheritance.md#Preventing-Overrides)。

- **`lazy`**：将此修饰符应用于类或结构体的存储变量属性，表示该属性的初始值在首次访问该属性时计算并存储，且最多计算一次。关于如何使用 `lazy` 修饰符的示例，参见 [延迟存储属性](properties.md#Lazy-Stored-Properties)。

- **`optional`**：将此修饰符应用于协议的属性、方法或下标成员，表示遵循类型不要求实现这些成员。

只能将 `optional` 修饰符应用于用 `objc` 特性标记的协议。因此，只有类类型才能采纳并遵循包含可选成员要求的协议。关于如何使用 `optional` 修饰符的更多信息，以及如何访问可选协议成员的指导——例如，当你不确定某个遵循类型是否实现了它们时——参见 [可选协议要求](protocols.md#Optional-Protocol-Requirements)。

- **`required`**：将此修饰符应用于类的指定构造器或便利构造器，表示每个子类都必须实现该构造器。子类对该构造器的实现也必须用 `required` 修饰符标记。

- **`static`**：将此修饰符应用于结构体、类、枚举或协议的成员，表示该成员是类型的成员，而不是该类型实例的成员。在类声明的作用域中，在成员声明上写 `static` 修饰符的效果等同于在该成员声明上写 `class` 和 `final` 修饰符。不过，类的常量类型属性是个例外：那里的 `static` 具有其通常的、非类含义，因为你不能在这些声明上写 `class` 或 `final`。

- **`unowned`**：将此修饰符应用于存储变量、常量或存储属性，表示该变量或属性对它存储为值的对象持有无主引用。如果你在对象被释放后尝试访问该变量或属性，会引发运行时错误。与弱引用一样，属性或值的类型必须是类类型；与弱引用不同，该类型是非可选的。关于 `unowned` 修饰符的示例和更多信息，参见 [无主引用](automatic-reference-counting.md#Unowned-References)。

- **`unowned(safe)`**：`unowned` 的显式写法。

- **`unowned(unsafe)`**：将此修饰符应用于存储变量、常量或存储属性，表示该变量或属性对它存储为值的对象持有无主引用。如果你在对象被释放后尝试访问该变量或属性，你访问的是该对象原先所在位置的内存，这是一种内存不安全的操作。与弱引用一样，属性或值的类型必须是类类型；与弱引用不同，该类型是非可选的。关于 `unowned` 修饰符的示例和更多信息，参见 [无主引用](automatic-reference-counting.md#Unowned-References)。

- **`weak`**：将此修饰符应用于存储变量或存储变量属性，表示该变量或属性对它存储为值的对象持有弱引用。该变量或属性的类型必须是可选类类型。如果你在对象被释放后访问该变量或属性，其值为 `nil`。关于 `weak` 修饰符的示例和更多信息，参见 [弱引用](automatic-reference-counting.md#Weak-References)。

### 访问控制级别 {#Access-Control-Levels}

Swift 提供五个级别的访问控制：open、public、internal、file private 和 private。你可以用下面某个访问级别修饰符标记声明，以指定该声明的访问级别。访问控制在 [访问控制](access-control.md) 中详细讨论。

- **`open`**：将此修饰符应用于声明，表示该声明可以被与声明同一模块中的代码访问和子类化。用 `open` 访问级别修饰符标记的声明，也可以被导入包含该声明的模块的模块中的代码访问和子类化。

- **`public`**：将此修饰符应用于声明，表示该声明可以被与声明同一模块中的代码访问和子类化。用 `public` 访问级别修饰符标记的声明，也可以被导入包含该声明的模块的模块中的代码访问（但不能子类化）。

- **`package`**：将此修饰符应用于声明，表示该声明只能被与声明同一个包中的代码访问。包是你在所用构建系统中定义的代码分发单元。当构建系统编译代码时，它通过向 Swift 编译器传递 `-package-name` 标志来指定包名。如果构建系统在构建两个模块时指定了相同的包名，那么这两个模块属于同一个包。

- **`internal`**：将此修饰符应用于声明，表示该声明只能被与声明同一模块中的代码访问。默认情况下，大多数声明隐式带有 `internal` 访问级别修饰符。

- **`fileprivate`**：将此修饰符应用于声明，表示该声明只能被与声明同一源文件中的代码访问。

- **`private`**：将此修饰符应用于声明，表示该声明只能被声明直接外围作用域内的代码访问。

就访问控制而言，扩展的行为如下：

- 如果同一个文件中有多个扩展，并且这些扩展都扩展同一个类型，那么所有这些扩展具有相同的访问控制作用域。这些扩展和它们所扩展的类型可以位于不同的文件中。

- 如果扩展与它所扩展的类型位于同一个文件中，该扩展具有与该类型相同的访问控制作用域。

- 在类型的声明中声明的私有成员，可以从该类型的扩展中访问。在一个扩展中声明的私有成员，可以从其他扩展以及被扩展类型的声明中访问。

上面每个访问级别修饰符都可以选择接受一个实参，该实参由括在括号中的 `set` 关键字组成——例如 `private(set)`。当你想为变量或下标的 setter 指定一个小于或等于该变量或下标自身访问级别的访问级别时，请使用这种形式的访问级别修饰符，详见 [getter 和 setter](access-control.md#Getters-and-Setters)。

> 声明修饰符语法：
>
> *declaration-modifier* → **`class`** | **`convenience`** | **`dynamic`** | **`final`** | **`infix`** | **`lazy`** | **`optional`** | **`override`** | **`postfix`** | **`prefix`** | **`required`** | **`static`** | **`unowned`** | **`unowned`** **`(`** **`safe`** **`)`** | **`unowned`** **`(`** **`unsafe`** **`)`** | **`weak`** \
> *declaration-modifier* → *access-level-modifier* \
> *declaration-modifier* → *mutation-modifier* \
> *declaration-modifier* → *actor-isolation-modifier* \
> *declaration-modifiers* → *declaration-modifier* *declaration-modifiers*_?_
>
> *access-level-modifier* → **`private`** | **`private`** **`(`** **`set`** **`)`** \
> *access-level-modifier* → **`fileprivate`** | **`fileprivate`** **`(`** **`set`** **`)`** \
> *access-level-modifier* → **`internal`** | **`internal`** **`(`** **`set`** **`)`** \
> *access-level-modifier* → **`package`** | **`package`** **`(`** **`set`** **`)`** \
> *access-level-modifier* → **`public`** | **`public`** **`(`** **`set`** **`)`** \
> *access-level-modifier* → **`open`** | **`open`** **`(`** **`set`** **`)`**
>
> *mutation-modifier* → **`mutating`** | **`nonmutating`**
>
> *actor-isolation-modifier* → **`nonisolated`**

