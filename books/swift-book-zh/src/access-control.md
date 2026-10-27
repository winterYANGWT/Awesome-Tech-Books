# 访问控制 {#Access-Control}

通过声明、文件和模块来管理代码的可见性。

*访问控制*（access control）限制其他源文件和模块中的代码访问你代码的某些部分。这个特性让你可以隐藏代码的实现细节，并指定一个首选的接口来访问和使用这些代码。

你可以为单个类型（类、结构体和枚举）指定特定的访问级别，也可以为属于这些类型的属性、方法、构造器和下标指定访问级别。协议可以被限制在某个特定上下文中使用，全局常量、变量和函数也是如此。

除了提供各种级别的访问控制，Swift 还通过为典型场景提供默认访问级别，减少了显式指定访问级别的需要。事实上，如果你在编写单目标 App，可能根本不需要显式指定访问级别。

> 注意：下文为了简洁，把可以应用访问控制的各种代码要素（属性、类型、函数等）统称为“实体”。

## 模块、源文件和包 {#Modules-Source-Files-and-Packages}

Swift 的访问控制模型基于模块、源文件和包这几个概念。

*模块*（module）是代码分发的一个独立单元——一个作为单一单元构建和发布的框架或 App，它可以被另一个模块用 Swift 的 `import` 关键字导入。

Xcode 中的每个构建目标（例如 App 包或框架）在 Swift 中都被视为一个独立的模块。如果你把 App 代码的某些部分组织成一个独立的框架——也许是为了封装这些代码并在多个 App 之间复用它——那么当这个框架被导入并在某个 App 中使用，或在另一个框架中使用时，你在该框架中定义的所有内容都会成为另一个独立模块的一部分。

*源文件*（source file）是模块中的单个 Swift 源代码文件（实际上就是 App 或框架中的单个文件）。虽然通常会把各个类型定义在单独的源文件中，但一个源文件也可以包含多个类型、函数等的定义。

*包*（package）是你作为一个单元开发的一组模块。组成一个包的模块是在配置你使用的构建系统时定义的，而不是在 Swift 源代码中定义的。例如，如果你使用 Swift Package Manager 构建代码，就在 `Package.swift` 文件中使用 [PackageDescription][] 模块的 API 来定义包；如果你使用 Xcode，则在 Package Access Identifier 构建设置中指定包名。

[PackageDescription]: https://developer.apple.com/documentation/packagedescription

## 访问级别 {#Access-Levels}

Swift 为代码中的实体提供了六种不同的*访问级别*。这些访问级别是相对于实体定义所在的源文件、该源文件所属的模块，以及该模块所属的包而言的。

- *开放访问*和*公开访问*让实体可以在其定义模块的任何源文件中使用，也可以在导入了该定义模块的另一个模块的源文件中使用。在指定框架的公开接口时，你通常使用开放访问或公开访问。开放访问和公开访问的区别见下文。
- *包访问*让实体可以在其定义包的任何源文件中使用，但不能在该包之外的任何源文件中使用。你通常会在划分为多个模块的 App 或框架内部使用包访问。
- *内部访问*让实体可以在其定义模块的任何源文件中使用，但不能在该模块之外的任何源文件中使用。在定义 App 或框架的内部结构时，你通常使用内部访问。
- *文件私有访问*把实体的使用限制在它自己所在的定义源文件中。当某个功能的实现细节要在整个文件中使用时，用文件私有访问来隐藏这些细节。
- *私有访问*把实体的使用限制在所在的声明中，以及同一文件中该声明的扩展中。当某个功能的实现细节只在单个声明内用到时，用私有访问来隐藏这些细节。

开放访问是最高（限制最少）的访问级别，私有访问是最低（限制最多）的访问级别。

开放访问只适用于类和类的成员，它与公开访问的区别在于允许模块外的代码继承并重写，详见 [子类化](access-control.md#Subclassing)。把一个类标记为开放访问，明确表示你已经考虑过其他模块的代码使用该类作为父类所带来的影响，并且已经相应地设计了该类的代码。

### 访问级别的指导原则 {#Guiding-Principle-of-Access-Levels}

Swift 中的访问级别遵循一个总体指导原则：*任何实体都不能用访问级别更低（限制更严）的另一个实体来定义*。

例如：

- 公开变量不能定义为具有内部、文件私有或私有类型，因为该类型可能并非在公开变量使用的所有地方都可用。
- 函数的访问级别不能高于其形参类型和返回类型的访问级别，因为函数可能会在其组成类型对周围代码不可用的场景中被使用。

下文详细介绍了这一指导原则对语言各个方面的具体影响。

### 默认访问级别 {#Default-Access-Levels}

如果你没有自己指定显式的访问级别，代码中的所有实体（本章后面会介绍少数例外情况）默认的访问级别都是内部访问。因此，在很多情况下你不需要在代码中显式指定访问级别。

### 单目标 App 的访问级别 {#Access-Levels-for-Single-Target-Apps}

编写简单的单目标 App 时，App 中的代码通常自包含在 App 内部，不需要在 App 的模块之外提供。默认的内部访问级别已经满足这一要求。因此，你不需要指定自定义的访问级别。不过，你可能希望把代码的某些部分标记为文件私有或私有，以便对 App 模块中的其他代码隐藏它们的实现细节。

### 框架的访问级别 {#Access-Levels-for-Frameworks}

开发框架时，把该框架面向公众的接口标记为开放访问或公开访问，以便其他模块（例如导入该框架的 App）可以查看和访问它。这个面向公众的接口就是该框架的应用程序编程接口（API）。

> 注意：框架的任何内部实现细节仍然可以使用默认的内部访问级别，或者如果你想把它们对框架内部代码的其他部分隐藏起来，也可以标记为私有或文件私有。只有当你希望某个实体成为框架 API 的一部分时，才需要把它标记为开放访问或公开访问。

### 单元测试目标的访问级别 {#Access-Levels-for-Unit-Test-Targets}

编写带单元测试目标的 App 时，App 中的代码需要对该模块可用，才能被测试。默认情况下，只有标记为开放访问或公开访问的实体才能被其他模块访问。不过，如果你用 `@testable` 特性标记产品模块的导入声明，并在启用测试的情况下编译该产品模块，单元测试目标就可以访问任何内部实体。

## 访问控制语法 {#Access-Control-Syntax}

要定义实体的访问级别，请把 [访问级别](access-control.md#Access-Levels) 中列出的某个修饰符（例如 `public` 或 `private`）放在实体声明的最前面。例如：

```swift
public class SomePublicClass {}
internal struct SomeInternalStruct() {}
private func somePrivateFunction() {}
```

上面的代码把 `SomePublicClass` 声明为公开访问，把 `SomeInternalStruct` 声明为内部访问，把 `somePrivateFunction()` 声明为私有访问。如果你不写显式的访问级别，默认的访问级别修饰符是 `internal`，如 [默认访问级别](access-control.md#Default-Access-Levels) 所述。例如，在下面的代码中，`SomeInternalStruct` 隐式地是内部访问：

```swift
struct SomeInternalStruct() {}
```

## 自定义类型 {#Custom-Types}

如果你想为自定义类型指定显式的访问级别，请在定义该类型时指定。之后，这个新类型就可以在其访问级别允许的任何地方使用。例如，如果你定义了一个文件私有类，那么这个类只能作为属性类型、函数形参或返回类型，在定义该文件私有类的源文件中使用。

类型的访问控制级别还会影响该类型*成员*（其属性、方法、构造器和下标）的默认访问级别。如果你把类型的访问级别定义为私有或文件私有，其成员的默认访问级别也会是私有或文件私有。如果你把类型的访问级别定义为内部或公开（或者不显式指定访问级别而使用默认的内部访问级别），该类型成员的默认访问级别就是内部访问。

> 重要：公开类型的成员默认是内部访问，而不是公开访问。如果你希望某个类型成员是公开访问的，必须显式地把它标记为公开。这一要求确保类型的公开接口是你主动选择发布的，也能避免误把类型的内部运作当作公开 API 暴露出去。

```swift
public class SomePublicClass {                   // explicitly public class
    public var somePublicProperty = 0            // explicitly public class member
    var someInternalProperty = 0                 // implicitly internal class member
    fileprivate func someFilePrivateMethod() {}  // explicitly file-private class member
    private func somePrivateMethod() {}          // explicitly private class member
}

class SomeInternalClass {                        // implicitly internal class
    var someInternalProperty = 0                 // implicitly internal class member
    fileprivate func someFilePrivateMethod() {}  // explicitly file-private class member
    private func somePrivateMethod() {}          // explicitly private class member
}

fileprivate class SomeFilePrivateClass {         // explicitly file-private class
    func someFilePrivateMethod() {}              // implicitly file-private class member
    private func somePrivateMethod() {}          // explicitly private class member
}

private class SomePrivateClass {                 // explicitly private class
    func somePrivateMethod() {}                  // implicitly private class member
}
```

### 元组类型 {#Tuple-Types}

元组类型的访问级别是元组中使用的所有类型中最严格的访问级别。例如，如果你用两个不同的类型组成元组，一个具有内部访问级别，另一个具有私有访问级别，那么这个复合元组类型的访问级别就是私有。

> 注意：元组类型不像类、结构体、枚举和函数那样有独立的定义。元组类型的访问级别由组成该元组类型的各个类型自动决定，不能显式指定。

### 函数类型 {#Function-Types}

函数类型的访问级别按该函数形参类型和返回类型中最严格的访问级别计算。如果函数计算出的访问级别与上下文默认的访问级别不匹配，你必须把访问级别显式指定为函数定义的一部分。

下面的例子定义了一个名为 `someFunction()` 的全局函数，但没有为该函数本身提供特定的访问级别修饰符。你可能期望这个函数具有默认的“内部”访问级别，但事实并非如此。实际上，按下面的写法，`someFunction()` 无法通过编译：

```swift
func someFunction() -> (SomeInternalClass, SomePrivateClass) {
    // function implementation goes here
}
```

该函数的返回类型是一个元组类型，由上面 [自定义类型](access-control.md#Custom-Types) 中定义的两个自定义类组成。其中一个类被定义为内部访问，另一个被定义为私有访问。因此，这个复合元组类型的整体访问级别是私有（即元组各组成类型中最低的访问级别）。

由于函数的返回类型是私有访问，你必须用 `private` 修饰符标记函数的整体访问级别，函数声明才是有效的：

```swift
private func someFunction() -> (SomeInternalClass, SomePrivateClass) {
    // function implementation goes here
}
```

用 `public` 或 `internal` 修饰符标记 `someFunction()` 的定义，或者使用默认的内部设置，都是无效的，因为该函数的公开或内部用户可能没有适当的访问权限来访问函数返回类型中使用的私有类。

### 枚举类型 {#Enumeration-Types}

枚举的各个成员自动获得与它们所属的枚举相同的访问级别。你不能为单个枚举成员指定不同的访问级别。

在下面的例子中，`CompassPoint` 枚举被显式指定为公开访问级别。因此，枚举成员 `north`、`south`、`east` 和 `west` 的访问级别也是公开：

```swift
public enum CompassPoint {
    case north
    case south
    case east
    case west
}
```

#### 原始值和关联值 {#Raw-Values-and-Associated-Values}

枚举定义中用于原始值或关联值的类型，其访问级别至少要跟枚举的访问级别一样高。例如，你不能把私有类型用作内部访问级别的枚举的原始值类型。

### 嵌套类型 {#Nested-Types}

嵌套类型的访问级别与其所在类型相同，除非所在类型是公开的。在公开类型中定义的嵌套类型，自动具有内部访问级别。如果你希望公开类型中的某个嵌套类型可以公开使用，必须显式地把该嵌套类型声明为公开。

## 子类化 {#Subclassing}

你可以继承任何在当前访问上下文中可访问、且与子类定义在同一模块中的类。你也可以继承其他模块中定义的任何开放类。子类的访问级别不能高于父类——例如，你不能写出内部父类的公开子类。

此外，对于定义在同一模块中的类，你可以重写在该访问上下文中可见的任何类成员（方法、属性、构造器或下标）。对于定义在其他模块中的类，你可以重写任何开放类成员。

重写可以让继承来的类成员比其父类版本具有更高的可访问性。在下面的例子中，类 `A` 是一个公开类，有一个名为 `someMethod()` 的文件私有方法。类 `B` 是 `A` 的子类，访问级别降为“内部”。尽管如此，类 `B` 提供了一个 `someMethod()` 的重写，访问级别为“内部”，这比 `someMethod()` 的原始实现*更高*：

```swift
public class A {
    fileprivate func someMethod() {}
}

internal class B: A {
    override internal func someMethod() {}
}
```

子类成员甚至可以调用访问权限低于该子类成员的父类成员，只要对父类成员的调用发生在允许的访问级别上下文中（也就是说，对于文件私有成员调用，要在与父类相同的源文件中进行；对于内部成员调用，要在与父类相同的模块中进行）：

```swift
public class A {
    fileprivate func someMethod() {}
}

internal class B: A {
    override internal func someMethod() {
        super.someMethod()
    }
}
```

由于父类 `A` 和子类 `B` 定义在同一个源文件中，`B` 中 `someMethod()` 的实现调用 `super.someMethod()` 是有效的。

## 常量、变量、属性和下标 {#Constants-Variables-Properties-and-Subscripts}

常量、变量或属性的可访问性不能高于其类型。例如，把公开属性声明为私有类型是无效的。同样，下标的可访问性不能高于其索引类型或返回类型。

如果常量、变量、属性或下标使用了私有类型，那么该常量、变量、属性或下标也必须标记为 `private`：

```swift
private var privateInstance = SomePrivateClass()
```

### getter 和 setter {#Getters-and-Setters}

常量、变量、属性和下标的 getter（取值器）和 setter（设置器）自动获得与它们所属的常量、变量、属性或下标相同的访问级别。

你可以让 setter 的访问级别比对应的 getter *更低*，以限制该变量、属性或下标的读写范围。要指定更低的访问级别，可以在 `var` 或 `subscript` 关键字之前写 `fileprivate(set)`、`private(set)`、`internal(set)` 或 `package(set)`。

> 注意：这条规则既适用于存储属性，也适用于计算属性。即使你没有为存储属性显式编写 getter 和 setter，Swift 仍会为你合成一个隐式的 getter 和 setter，用来访问该存储属性的底层存储。用 `fileprivate(set)`、`private(set)`、`internal(set)` 和 `package(set)` 可以改变这个合成 setter 的访问级别，方式与计算属性中显式 setter 完全相同。

下面的例子定义了一个名为 `TrackedString` 的结构体，用来跟踪某个字符串属性被修改的次数：

```swift
struct TrackedString {
    private(set) var numberOfEdits = 0
    var value: String = "" {
        didSet {
            numberOfEdits += 1
        }
    }
}
```

`TrackedString` 结构体定义了一个名为 `value` 的存储字符串属性，初始值为 `""`（空字符串）。该结构体还定义了一个名为 `numberOfEdits` 的存储整数属性，用来跟踪 `value` 被修改的次数。这种修改跟踪是通过 `value` 属性上的 `didSet` 属性观察器实现的，每次把 `value` 属性设为新值时，它都会让 `numberOfEdits` 递增。

`TrackedString` 结构体和 `value` 属性都没有提供显式的访问级别修饰符，因此它们都获得默认的内部访问级别。不过，`numberOfEdits` 属性的访问级别用 `private(set)` 修饰符标记，表示该属性的 getter 仍具有默认的内部访问级别，但只能在属于 `TrackedString` 结构体的代码内部设置该属性。这让 `TrackedString` 可以在内部修改 `numberOfEdits` 属性，而在结构体定义之外使用时把该属性呈现为只读属性。

如果你创建一个 `TrackedString` 实例，并多次修改它的字符串值，就能看到 `numberOfEdits` 属性值更新为与修改次数一致：

```swift
var stringToEdit = TrackedString()
stringToEdit.value = "This string will be tracked."
stringToEdit.value += " This edit will increment numberOfEdits."
stringToEdit.value += " So will this one."
print("The number of edits is \(stringToEdit.numberOfEdits)")
// Prints "The number of edits is 3".
```

虽然你可以在另一个源文件中查询 `numberOfEdits` 属性的当前值，但不能从另一个源文件中*修改*该属性。这一限制保护了 `TrackedString` 编辑跟踪功能的实现细节，同时仍然方便地提供了对该功能某一方面的访问。

注意，如有需要，你可以为 getter 和 setter 分别指定显式的访问级别。下面的例子展示了 `TrackedString` 结构体的另一个版本，其中结构体被定义为显式的公开访问级别。因此，该结构体的成员（包括 `numberOfEdits` 属性）默认具有内部访问级别。通过组合 `public` 和 `private(set)` 访问级别修饰符，你可以让该结构体的 `numberOfEdits` 属性 getter 为公开访问，而属性 setter 为私有访问：

```swift
public struct TrackedString {
    public private(set) var numberOfEdits = 0
    public var value: String = "" {
        didSet {
            numberOfEdits += 1
        }
    }
    public init() {}
}
```

## 构造器 {#Initializers}

自定义构造器可以被赋予小于或等于它所构造的类型的访问级别。唯一的例外是必要构造器（如 [必要构造器](initialization.md#Required-Initializers) 中所定义）。必要构造器的访问级别必须与它所属的类相同。

与函数和方法的形参一样，构造器形参的类型不能比构造器自身的访问级别更私有。

### 默认构造器 {#Default-Initializers}

如 [默认构造器](initialization.md#Default-Initializers) 所述，对于所有属性都提供了默认值、并且自身没有提供至少一个构造器的任何结构体或基类，Swift 会自动提供一个不带任何实参的*默认构造器*。

默认构造器具有与被构造类型相同的访问级别，除非该类型被定义为 `public`。对于定义为 `public` 的类型，默认构造器被视为内部访问。如果你希望公开类型在其他模块中使用时可以用无实参构造器构造，就必须在类型定义中自己显式提供一个公开的无实参构造器。

### 结构体类型的默认成员逐一构造器 {#Default-Memberwise-Initializers-for-Structure-Types}

如果结构体的任一存储属性是私有的，该结构体类型的默认成员逐一构造器就被视为私有访问。同样，如果结构体的任一存储属性是文件私有，该构造器就是文件私有。否则，该构造器具有内部访问级别。

与上面的默认构造器一样，如果你希望公开的结构体类型在其他模块中使用时可以用成员逐一构造器构造，就必须在类型定义中自己提供一个公开的成员逐一构造器。

## 协议 {#Protocols}

如果你想为协议类型指定显式的访问级别，请在定义协议时指定。这样你就能创建只能在某个特定访问上下文中采纳的协议。

协议定义中每个要求的访问级别都会自动设为与该协议相同的访问级别。你不能把协议要求的访问级别设为不同于它所支持的协议的访问级别。这确保了协议的所有要求在任何采纳该协议的类型上都可见。

> 注意：如果你定义了一个公开协议，那么这些协议要求在实现时必须是公开访问级别。这一行为与其他类型不同：对于其他类型，公开的类型定义意味着该类型的成员是内部访问级别。

### 协议继承 {#Protocol-Inheritance}

如果你定义的新协议继承自一个已有协议，新协议的访问级别最高只能与它所继承的协议相同。例如，你不能写出继承内部协议的公开协议。

### 协议一致性 {#Protocol-Conformance}

类型可以遵循访问级别低于该类型本身的协议。例如，你可以定义一个公开类型，它可以在其他模块中使用，但它对某个内部协议的一致性只能在该内部协议的定义模块内使用。

类型遵循某个特定协议的上下文，是该类型访问级别与协议访问级别中的较小者。例如，如果类型是公开的，而它遵循的某个协议是内部的，那么该类型对该协议的一致性也是内部的。

当你编写或扩展一个类型使其遵循协议时，必须确保该类型对每个协议要求的实现至少具有与该类型对该协议的一致性相同的访问级别。例如，如果公开类型遵循内部协议，那么该类型对每个协议要求的实现至少必须是内部访问。

> 注意：在 Swift 中，与 Objective-C 一样，协议一致性是全局的——同一个类型不可能在同一个程序内以两种不同的方式遵循同一个协议。

## 扩展 {#Extensions}

你可以在类、结构体或枚举可用的任何访问上下文中扩展它们。在扩展中添加的任何类型成员，其默认访问级别与在被扩展的原始类型中声明的类型成员相同。如果你扩展的是公开或内部类型，添加的任何新类型成员默认具有内部访问级别。如果你扩展的是文件私有类型，添加的任何新类型成员默认具有文件私有访问级别。如果你扩展的是私有类型，添加的任何新类型成员默认具有私有访问级别。

或者，你可以用显式的访问级别修饰符（例如 `private`）标记扩展，为扩展中定义的所有成员设置新的默认访问级别。在扩展内部，仍然可以为单个类型成员覆盖这个新的默认访问级别。

如果你用扩展来添加协议一致性，就不能为该扩展提供显式的访问级别修饰符。此时，协议自身的访问级别会被用来为扩展中每个协议要求的实现提供默认访问级别。

### 扩展中的私有成员 {#Private-Members-in-Extensions}

与被扩展的类、结构体或枚举位于同一文件中的扩展，其行为就像扩展中的代码是作为原始类型声明的一部分编写的一样。因此，你可以：

- 在原始声明中声明一个私有成员，并从同一文件中的扩展访问该成员。
- 在一个扩展中声明一个私有成员，并从同一文件中的另一个扩展访问该成员。
- 在扩展中声明一个私有成员，并从同一文件中的原始声明访问该成员。

这种行为意味着，无论你的类型是否有私有实体，你都可以用同样的方式使用扩展来组织代码。例如，给定下面这个简单的协议：

```swift
protocol SomeProtocol {
    func doSomething()
}
```

你可以用扩展来添加协议一致性，像这样：

```swift
struct SomeStruct {
    private var privateVariable = 12
}

extension SomeStruct: SomeProtocol {
    func doSomething() {
        print(privateVariable)
    }
}
```

## 泛型 {#Generics}

泛型类型或泛型函数的访问级别，是泛型类型或函数自身的访问级别与其类型形参上任何类型约束的访问级别中的较小者。

## 类型别名 {#Type-Aliases}

就访问控制而言，你定义的任何类型别名都被视为不同的类型。类型别名的访问级别可以小于或等于它所别名类型的访问级别。例如，私有类型别名可以给私有、文件私有、内部、公开或开放类型起别名，但公开类型别名不能给内部、文件私有或私有类型起别名。

> 注意：这条规则也适用于那些用来满足协议一致性的关联类型的类型别名。
