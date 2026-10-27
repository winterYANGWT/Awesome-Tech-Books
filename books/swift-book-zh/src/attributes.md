# 特性 {#Attributes}

为声明和类型添加信息。

Swift 中的特性有两种：适用于声明的特性，以及适用于类型的特性。特性提供关于声明或类型的附加信息。例如，函数声明上的 `discardableResult` 特性表示：尽管该函数会返回一个值，但如果返回值未被使用，编译器不应生成警告。

写特性时，先写 `@` 符号，再写特性的名称以及该特性接受的任何实参：

```swift
@<#attribute name#>
@<#attribute name#>(<#attribute arguments#>)
```

有些声明特性接受实参，用于进一步说明该特性的信息以及它如何应用于特定的声明。这些*特性实参*（attribute arguments）括在圆括号中，其格式由它们所属的特性定义。

附加宏和属性包装器也使用特性语法。关于宏如何展开，请参阅 [宏展开表达式](expressions.md#Macro-Expansion-Expression)。关于属性包装器，请参阅 [propertyWrapper](attributes.md#propertyWrapper)。

## 声明特性 {#Declaration-Attributes}

声明特性只能应用于声明。

### attached {#attached}

将 `attached` 特性应用于宏声明。该特性的实参指明宏的角色。对于具有多个角色的宏，请多次应用 `attached` 宏，每个角色一次。

该特性的第一个实参指明宏的角色：

- **同伴宏**：把 `peer` 写为该特性的第一个实参。实现该宏的类型遵循 `PeerMacro` 协议。这类宏在与它所附加到的声明相同的作用域中产生新的声明。例如，把同伴宏应用于结构体的某个方法，可以在该结构体上定义额外的方法和属性。

- **成员宏**：把 `member` 写为该特性的第一个实参。实现该宏的类型遵循 `MemberMacro` 协议。这类宏产生新的声明，它们是该宏所附加到的类型或扩展的成员。例如，把成员宏应用于结构体声明，可以在该结构体上定义额外的方法和属性。

- **成员特性**：把 `memberAttribute` 写为该特性的第一个实参。实现该宏的类型遵循 `MemberAttributeMacro` 协议。这类宏为它所附加到的类型或扩展的成员添加特性。

- **访问器宏**：把 `accessor` 写为该特性的第一个实参。实现该宏的类型遵循 `AccessorMacro` 协议。这类宏为它们所附加到的存储属性添加访问器，使其成为计算属性。

- **扩展宏**：把 `extension` 写为该特性的第一个实参。实现该宏的类型遵循 `ExtensionMacro` 协议。这类宏可以添加协议一致性、`where` 子句，以及作为该宏所附加到的类型的成员的新声明。如果宏添加协议一致性，请包含 `conformances:` 实参并指定这些协议。一致性列表包含协议名称、引用一致性列表项的类型别名，或一致性列表项的协议组合。嵌套类型上的扩展宏会展开为该文件顶层的扩展。你不能在扩展、类型别名或嵌套在函数内部的类型上编写扩展宏，也不能使用扩展宏添加带有同伴宏的扩展。

- **主体宏**：把 `body` 写为该特性的第一个实参。实现该宏的类型遵循 `BodyMacro` 协议。这类宏可以生成、修改或替换它们所附加到的函数或访问器的主体。在任何给定函数上最多只能编写一个主体宏。如果提供了原始函数体，它在语法上必须正确，但在宏展开之后才会进行类型检查。

同伴宏和成员宏角色需要 `names:` 实参，列出该宏生成的符号的名称。如果访问器宏生成 `willSet` 或 `didSet` 属性观察器，那么该访问器宏角色需要 `names:` 实参。生成属性观察器的访问器宏不能添加其他访问器，因为观察器只适用于存储属性。如果扩展宏在扩展内部添加声明，那么该扩展宏角色也需要 `names:` 实参。当宏声明包含 `names:` 实参时，宏实现只能生成名称与该列表匹配的符号。不过，宏不必为列出的每个名称都生成一个符号。该实参的值是一个列表，包含以下一项或多项：

- `named(<#name#>)`，其中 *name* 是该固定符号名称，用于事先已知的名称。

- `overloaded`，用于与现有符号相同的名称。

- `prefixed(<#prefix#>)`，其中 *prefix* 被添加到符号名称之前，用于以固定字符串开头的名称。

- `suffixed(<#suffix#>)`，其中 *suffix* 被追加到符号名称之后，用于以固定字符串结尾的名称。

- `arbitrary`，用于在宏展开之前无法确定的名称。

作为一种特殊情况，对于行为类似于属性包装器的宏，可以写 `prefixed($)`。

### available {#available}

应用此特性以指明声明相对于特定 Swift 语言版本或特定平台及操作系统版本的生命周期。

`available` 特性总是与一个包含两个或更多以逗号分隔的特性实参的列表一起出现。这些实参以以下平台名称或语言名称之一开头：

- `iOS`
- `iOSApplicationExtension`
- `macOS`
- `macOSApplicationExtension`
- `macCatalyst`
- `macCatalystApplicationExtension`
- `watchOS`
- `watchOSApplicationExtension`
- `tvOS`
- `tvOSApplicationExtension`
- `visionOS`
- `visionOSApplicationExtension`
- `swift`

你也可以使用星号（`*`）来指明该声明在上述所有平台名称上的可用性。使用 Swift 版本号指定可用性的 `available` 特性不能使用星号。

其余实参可以按任意顺序出现，用于指定关于声明生命周期的附加信息，包括重要的里程碑。

- `unavailable` 实参表示该声明在指定平台上不可用。在指定 Swift 版本可用性时不能使用此实参。
- `introduced` 实参指明引入该声明的指定平台或语言的第一个版本。它的形式如下：

  ```swift
  introduced: <#version number#>
  ```
*version number* 由一个至三个正整数组成，用句点分隔。
- `deprecated` 实参指明弃用该声明的指定平台或语言的第一个版本。它的形式如下：

  ```swift
  deprecated: <#version number#>
  ```
可选的 *version number* 由一个至三个正整数组成，用句点分隔。省略版本号表示该声明当前已弃用，但不提供弃用发生时间的任何信息。如果省略版本号，也要省略冒号（`:`）。
- `obsoleted` 实参指明废弃该声明的指定平台或语言的第一个版本。声明被废弃后，会从指定平台或语言中移除，不能再使用。它的形式如下：

  ```swift
  obsoleted: <#version number#>
  ```
*version number* 由一个至三个正整数组成，用句点分隔。

- `noasync` 实参表示所声明的符号不能直接在异步上下文中使用。

因为 Swift 并发在潜在的挂起点之后可能在不同的线程上恢复，所以在跨越挂起点时使用线程局部存储、锁、互斥量或信号量等元素，可能会导致错误的结果。

为避免这个问题，请在符号的声明中添加 `@available(*, noasync)` 特性：

  ```swift
  extension pthread_mutex_t {

    @available(*, noasync)
    mutating func lock() {
        pthread_mutex_lock(&self)
    }

    @available(*, noasync)
    mutating func unlock() {
        pthread_mutex_unlock(&self)
    }
  }
  ```

当有人在异步上下文中使用该符号时，此特性会引发编译时错误。你也可以使用 `message` 实参来提供关于该符号的附加信息。

  ```swift
  @available(*, noasync, message: "Migrate locks to Swift concurrency.")
  mutating func lock() {
    pthread_mutex_lock(&self)
  }
  ```

如果你能保证代码以安全的方式使用可能不安全的符号，可以把它包装在一个同步函数中，并从异步上下文调用该函数。

  ```swift

  // Provide a synchronous wrapper around methods with a noasync declaration.
  extension pthread_mutex_t {
    mutating func withLock(_ operation: () -> ()) {
      self.lock()
      operation()
      self.unlock()
    }
  }

  func downloadAndStore(key: Int,
                      dataStore: MyKeyedStorage,
                      dataLock: inout pthread_mutex_t) async {
    // Safely call the wrapper in an asynchronous context.
    dataLock.withLock {
      dataStore[key] = downloadContent()
    }
  }
  ```

你可以在大多数声明上使用 `noasync` 实参；但是，不能在声明析构器时使用它。Swift 必须能够从任何上下文（同步和异步）调用类的析构器。

- `message` 实参提供一个文本消息，当编译器就使用标记为 `deprecated`、`obsoleted` 或 `noasync` 的声明发出警告或错误时会显示该消息。它的形式如下：

  ```swift
  message: <#message#>
  ```
*message* 由一个字符串字面量组成。
- `renamed` 实参提供一个文本消息，指明已重命名的声明的新名称。当编译器就使用已重命名的声明发出错误时会显示新名称。它的形式如下：

  ```swift
  renamed: <#new name#>
  ```
*new name* 由一个字符串字面量组成。

可以将带有 `renamed` 和 `unavailable` 实参的 `available` 特性应用于类型别名声明（如下所示），以表示某个声明的名称在框架或库的不同版本之间发生了变化。这种组合会产生一个编译时错误，表明该声明已被重命名。

  ```swift
  // First release
  protocol MyProtocol {
      // protocol definition
  }
  ```

  ```swift
  // Subsequent release renames MyProtocol
  protocol MyRenamedProtocol {
      // protocol definition
  }

  @available(*, unavailable, renamed: "MyRenamedProtocol")
  typealias MyProtocol = MyRenamedProtocol
  ```

你可以在单个声明上应用多个 `available` 特性，以指定该声明在不同平台和不同 Swift 版本上的可用性。如果 `available` 特性指定的平台或语言版本与当前目标不匹配，则该特性所适用的声明会被忽略。如果使用多个 `available` 特性，有效可用性是平台可用性与 Swift 可用性的组合。

如果 `available` 特性除平台名称或语言名称实参外只指定了 `introduced` 实参，则可以改用以下简写语法：

```swift
@available(<#platform name#> <#version number#>, *)
@available(swift <#version number#>)
```

`available` 特性的简写语法可以简洁地表达多个平台上的可用性。尽管这两种形式在功能上等价，但只要可能，就优先使用简写形式。

```swift
@available(iOS 10.0, macOS 10.12, *)
class MyClass {
    // class definition
}
```

使用 Swift 版本号指定可用性的 `available` 特性不能另外指定声明的平台可用性。请改用单独的 `available` 特性来指定 Swift 版本可用性以及一个或多个平台可用性。

```swift
@available(swift 3.0.2)
@available(macOS 10.12, *)
struct MyStruct {
    // struct definition
}
```

### backDeployed {#backDeployed}

将此特性应用于函数、方法、下标或计算属性，以便在调用或访问该符号的程序中包含该符号实现的一份副本。你可以使用此特性来标注作为平台一部分发布的符号，例如操作系统附带的 API。此特性标记的符号可以通过在访问它们的程序中包含其实现副本而被追溯地提供。复制实现也称为*发送到客户端*（emitting into the client）。

此特性接受一个 `before:` 实参，指定提供该符号的平台的首个版本。这些平台版本的含义与为 `available` 特性指定的平台版本相同。与 `available` 特性不同，该列表不能包含用于指代所有版本的星号（`*`）。例如，考虑以下代码：

```swift
@available(iOS 16, *)
@backDeployed(before: iOS 17)
func someFunction() { /* ... */ }
```

在上面的示例中，iOS SDK 从 iOS 17 开始提供 `someFunction()`。此外，该 SDK 还使用向后部署（back deployment）让 `someFunction()` 在 iOS 16 上可用。

编译调用此函数的代码时，Swift 会插入一层间接调用，用于找到该函数的实现。如果代码运行时使用的 SDK 版本包含此函数，则使用 SDK 的实现。否则，使用调用方中包含的副本。在上面的示例中，调用 `someFunction()` 时，在 iOS 17 或更高版本上运行会使用 SDK 中的实现；在 iOS 16 上运行时则使用调用方中包含的 `someFunction()` 副本。

> 注意：当调用方的最低部署目标等于或高于包含该符号的 SDK 首个版本时，编译器可以优化掉运行时检查，直接调用 SDK 的实现。在这种情况下，如果你直接访问向后部署的符号，编译器还可以从客户端中省略该符号实现的副本。

满足以下条件的函数、方法、下标和计算属性可以向后部署：

- 声明是 `public` 或 `@usableFromInline`。
- 对于类实例方法和类类型方法，该方法标记为 `final` 且未标记为 `@objc`。
- 实现满足可内联函数的要求，详见 [inlinable](attributes.md#inlinable)。

### discardableResult {#discardableResult}

将此特性应用于函数或方法声明，以便在调用返回值的函数或方法而未使用其结果时，抑制编译器警告。

### dynamicCallable {#dynamicCallable}

将此特性应用于类、结构体、枚举或协议，以将该类型的实例当作可调用的函数来处理。该类型必须实现 `dynamicallyCall(withArguments:)` 方法或 `dynamicallyCall(withKeywordArguments:)` 方法，或两者都实现。

你可以像调用接受任意数量实参的函数一样，调用动态可调用类型的实例。

```swift
@dynamicCallable
struct TelephoneExchange {
    func dynamicallyCall(withArguments phoneNumber: [Int]) {
        if phoneNumber == [4, 1, 1] {
            print("Get Swift help on forums.swift.org")
        } else {
            print("Unrecognized number")
        }
    }
}

let dial = TelephoneExchange()

// Use a dynamic method call.
dial(4, 1, 1)
// Prints "Get Swift help on forums.swift.org".

dial(8, 6, 7, 5, 3, 0, 9)
// Prints "Unrecognized number".

// Call the underlying method directly.
dial.dynamicallyCall(withArguments: [4, 1, 1])
```

`dynamicallyCall(withArguments:)` 方法的声明必须只有一个形参，且该形参遵循 [`ExpressibleByArrayLiteral`](https://developer.apple.com/documentation/swift/expressiblebyarrayliteral) 协议——就像上面示例中的 `[Int]`。返回类型可以是任意类型。

如果你实现了 `dynamicallyCall(withKeywordArguments:)` 方法，就可以在动态方法调用中包含标签。

```swift
@dynamicCallable
struct Repeater {
    func dynamicallyCall(withKeywordArguments pairs: KeyValuePairs<String, Int>) -> String {
        return pairs
            .map { label, count in
                repeatElement(label, count: count).joined(separator: " ")
            }
            .joined(separator: "\n")
    }
}

let repeatLabels = Repeater()
print(repeatLabels(a: 1, b: 2, c: 3, b: 2, a: 1))
// a
// b b
// c c c
// b b
// a
```

`dynamicallyCall(withKeywordArguments:)` 方法的声明必须只有一个形参，且该形参遵循 [`ExpressibleByDictionaryLiteral`](https://developer.apple.com/documentation/swift/expressiblebydictionaryliteral) 协议，返回类型可以是任意类型。该形参的 [`Key`](https://developer.apple.com/documentation/swift/expressiblebydictionaryliteral/key) 必须是 [`ExpressibleByStringLiteral`](https://developer.apple.com/documentation/swift/expressiblebystringliteral)。上一个示例使用 [`KeyValuePairs`](https://developer.apple.com/documentation/swift/keyvaluepairs) 作为形参类型，以便调用方可以包含重复的形参标签——`a` 和 `b` 在对 `repeat` 的调用中出现了多次。

如果两个 `dynamicallyCall` 方法都实现了，那么当方法调用包含关键字实参时，会调用 `dynamicallyCall(withKeywordArguments:)`。在所有其他情况下，调用 `dynamicallyCall(withArguments:)`。

调用动态可调用实例时，实参和返回值必须与你某个 `dynamicallyCall` 方法实现中指定的类型匹配。下面示例中的调用无法编译，因为没有接受 `KeyValuePairs<String, String>` 的 `dynamicallyCall(withArguments:)` 实现。

```swift
repeatLabels(a: "four") // Error
```

### dynamicMemberLookup {#dynamicMemberLookup}

将此特性应用于类、结构体、枚举或协议，以便在运行时按名称查找成员。该类型必须实现 `subscript(dynamicMember:)` 下标。

在显式成员表达式中，如果命名成员没有对应的声明，该表达式会被理解为对该类型的 `subscript(dynamicMember:)` 下标的调用，并把有关该成员的信息作为实参传入。该下标可以接受 key path 或成员名称作为形参；如果两个下标都实现了，则使用接受 key path 实参的那个下标。

`subscript(dynamicMember:)` 的实现可以使用 [`KeyPath`](https://developer.apple.com/documentation/swift/keypath)、[`WritableKeyPath`](https://developer.apple.com/documentation/swift/writablekeypath) 或 [`ReferenceWritableKeyPath`](https://developer.apple.com/documentation/swift/referencewritablekeypath) 类型的实参来接受 key path。它可以使用遵循 [`ExpressibleByStringLiteral`](https://developer.apple.com/documentation/swift/expressiblebystringliteral) 协议的类型（大多数情况下是 `String`）的实参来接受成员名称。该下标的返回类型可以是任意类型。

按成员名称进行的动态成员查找可用于围绕无法在编译时进行类型检查的数据创建包装类型，例如把其他语言的数据桥接到 Swift 时。例如：

```swift
@dynamicMemberLookup
struct DynamicStruct {
    let dictionary = ["someDynamicMember": 325,
                      "someOtherMember": 787]
    subscript(dynamicMember member: String) -> Int {
        return dictionary[member] ?? 1054
    }
}
let s = DynamicStruct()

// Use dynamic member lookup.
let dynamic = s.someDynamicMember
print(dynamic)
// Prints "325".

// Call the underlying subscript directly.
let equivalent = s[dynamicMember: "someDynamicMember"]
print(dynamic == equivalent)
// Prints "true".
```

按 key path 进行的动态成员查找可用于以支持编译时类型检查的方式实现包装类型。例如：

```swift
struct Point { var x, y: Int }

@dynamicMemberLookup
struct PassthroughWrapper<Value> {
    var value: Value
    subscript<T>(dynamicMember member: KeyPath<Value, T>) -> T {
        get { return value[keyPath: member] }
    }
}

let point = Point(x: 381, y: 431)
let wrapper = PassthroughWrapper(value: point)
print(wrapper.x)
```

### export {#export}

将此特性应用于函数或方法声明，以控制其定义如何导出到客户端模块。请包含以下实参之一，以指明要导出声明的哪个方面：

- `interface` 实参指定只以可调用符号的形式向客户端导出接口。定义（函数体）不提供给客户端用于内联、优化或任何其他目的。使用此实参可以向客户端隐藏实现。

- `implementation` 实参指定只向客户端导出定义（函数体）。此函数不会在二进制文件中生成符号，客户端负责在需要的地方生成该定义的副本。使用此实参可以在不影响应用二进制接口（ABI）的情况下引入新函数或方法。

### freestanding {#freestanding}

将 `freestanding` 特性应用于独立宏的声明。

### frozen {#frozen}

将此特性应用于结构体或枚举声明，以限制可以对该类型进行的更改种类。此特性仅在库演进模式下编译时允许使用。库的未来版本不能通过添加、移除或重新排列枚举的成员或结构体的存储实例属性来更改该声明。这些更改在非冻结类型上是允许的，但会破坏冻结类型的 ABI 兼容性。

在库演进模式下，与非冻结结构体和枚举的成员交互的代码会以一种特殊方式编译：即使库的未来版本添加、移除或重新排列了该类型的某些成员，该代码也能继续工作而无需重新编译。编译器通过运行时查找信息和添加一层间接调用等技术来实现这一点。把结构体或枚举标记为冻结会放弃这种灵活性以换取性能：库的未来版本只能对该类型做有限的更改，但编译器可以在与该类型成员交互的代码中进行额外的优化。

冻结类型、冻结结构体的存储属性的类型，以及冻结枚举成员的关联值，必须是 `public` 的，或标记了 `usableFromInline` 特性。冻结结构体的属性不能有属性观察器，并且为存储实例属性提供初始值的表达式必须遵循与可内联函数相同的限制，详见 [inlinable](attributes.md#inlinable)。

要在命令行上启用库演进模式，请向 Swift 编译器传递 `-enable-library-evolution` 选项。要在 Xcode 中启用它，请把“Build Libraries for Distribution”构建设置（`BUILD_LIBRARY_FOR_DISTRIBUTION`）设为 Yes，如 [Xcode Help](https://help.apple.com/xcode/mac/current/#/dev04b3a04ba) 中所述。

对冻结枚举进行 switch 时不要求有 `default` 分支，如 [对将来新增的枚举成员进行切换](statements.md#Switching-Over-Future-Enumeration-Cases) 中所述。在对冻结枚举进行 switch 时包含 `default` 或 `@unknown default` 分支会产生警告，因为该代码永远不会执行。

### GKInspectable {#GKInspectable}

应用此特性可将自定义的 GameplayKit 组件属性暴露给 SpriteKit 编辑器 UI。应用此特性还隐含 `objc` 特性。

### globalActor {#globalActor}

将此特性应用于 actor、结构体、枚举或 final 类。该类型必须定义一个名为 `shared` 的静态属性，它提供该 actor 的共享实例。

全局 actor 把 actor 隔离的概念推广到分散在代码中多个不同位置（例如多个类型、文件和模块）的状态，并使得从并发代码安全访问全局变量成为可能。全局 actor 通过其 `shared` 属性的值提供的 actor 会串行化对所有这种状态的访问。你也可以使用全局 actor 来为并发代码中的约束建模，例如所有代码都需要在同一线程上执行这样的约束。

全局 actor 隐式遵循 [`GlobalActor`][] 协议。主 actor 是标准库提供的一个全局 actor，如 [主 actor](concurrency.md#The-Main-Actor) 中所述。大多数代码可以使用主 actor，而不必定义新的全局 actor。

[`GlobalActor`]: https://developer.apple.com/documentation/swift/globalactor

### inlinable {#inlinable}

将此特性应用于函数、方法、计算属性、下标、便利构造器或析构器声明，以把该声明的实现作为模块公开接口的一部分暴露出来。编译器可以将对可内联符号的调用替换为该符号实现在调用点的一份副本。

可内联代码可以与任何模块中声明的 `open` 和 `public` 符号交互，也可以与同一模块中声明并标记了 `usableFromInline` 特性的 `internal` 符号交互。可内联代码不能与 `private` 或 `fileprivate` 符号交互。

此特性不能应用于嵌套在函数内部的声明，也不能应用于 `fileprivate` 或 `private` 声明。在可内联函数内部定义的函数和闭包隐式地可内联，即使它们不能用此特性标记。

### main {#main}

将此特性应用于结构体、类或枚举声明，以表示它包含程序流程的顶层入口点。该类型必须提供一个不接受任何实参并返回 `Void` 的 `main` 类型函数。例如：

```swift
@main
struct MyTopLevel {
    static func main() {
        // Top-level code goes here
    }
}
```

描述 `main` 特性要求的另一种方式是：你为其写此特性的类型必须满足与遵循以下假设协议的类型相同的要求：

```swift
protocol ProvidesMain {
    static func main() throws
}
```

你编译为可执行文件的 Swift 代码最多只能包含一个顶层入口点，如 [顶层代码](declarations.md#Top-Level-Code) 中所述。

### nonobjc {#nonobjc}

将此特性应用于方法、属性、下标或构造器声明，以抑制隐式的 `objc` 特性。`nonobjc` 特性告诉编译器让该声明在 Objective-C 代码中不可用，即使它有可能在 Objective-C 中表示。

将此特性应用于扩展，与将它应用于该扩展中每个未显式标记 `objc` 特性的成员效果相同。

你可以使用 `nonobjc` 特性来解决标记了 `objc` 特性的类中桥接方法的循环依赖问题，并允许在标记了 `objc` 特性的类中重载方法和构造器。

标记了 `nonobjc` 特性的方法不能重写标记了 `objc` 特性的方法。但是，标记了 `objc` 特性的方法可以重写标记了 `nonobjc` 特性的方法。同样，标记了 `nonobjc` 特性的方法不能满足针对标记了 `objc` 特性的方法的协议要求。

### NSApplicationMain {#NSApplicationMain}

> 已弃用：此特性已弃用；请改用 [main](attributes.md#main) 特性。在 Swift 6 中，使用此特性会产生编译时错误。

将此特性应用于类，以表示它是应用委托。使用此特性等同于调用 `NSApplicationMain(_:_:)` 函数。

如果不使用此特性，请提供一个 `main.swift` 文件，并在顶层包含如下调用 `NSApplicationMain(_:_:)` 函数的代码：

```swift
import AppKit
NSApplicationMain(CommandLine.argc, CommandLine.unsafeArgv)
```

你编译为可执行文件的 Swift 代码最多只能包含一个顶层入口点，如 [顶层代码](declarations.md#Top-Level-Code) 中所述。

### NSCopying {#NSCopying}

将此特性应用于类的存储变量属性。此特性会使该属性的 setter 被合成，使用该属性值的一份*副本*（由 `copyWithZone(_:)` 方法返回），而不是属性值本身。该属性的类型必须遵循 `NSCopying` 协议。

`NSCopying` 特性的行为类似于 Objective-C 的 `copy` 属性特性。

### NSManaged {#NSManaged}

将此特性应用于继承自 `NSManagedObject` 的类的实例方法或存储变量属性，以表示 Core Data 会在运行时根据关联的实体描述动态提供其实现。对于标记了 `NSManaged` 特性的属性，Core Data 还会在运行时提供存储。应用此特性还隐含 `objc` 特性。

### objc {#objc}

将此特性应用于任何可以在 Objective-C 中表示的声明——例如，非嵌套类、协议、非泛型枚举（限制为整数原始值类型）、类的属性和方法（包括 getter 和 setter）、协议及协议的可选成员、构造器和下标。`objc` 特性告诉编译器某个声明可以在 Objective-C 代码中使用。

将此特性应用于扩展，与将它应用于该扩展中每个未显式标记 `nonobjc` 特性的成员效果相同。

编译器会隐式地向任何在 Objective-C 中定义的类的子类添加 `objc` 特性。但是，该子类不能是泛型的，也不能继承自任何泛型类。你可以向满足这些条件的子类显式添加 `objc` 特性，以指定其 Objective-C 名称（如下所述）。标记了 `objc` 特性的协议不能继承自未标记此特性的协议。

在以下情况下也会隐式添加 `objc` 特性：

- 声明是子类中的重写，并且父类的声明具有 `objc` 特性。
- 声明满足了具有 `objc` 特性的协议的要求。
- 声明具有 `IBAction`、`IBSegueAction`、`IBOutlet`、`IBDesignable`、`IBInspectable`、`NSManaged` 或 `GKInspectable` 特性。

如果将 `objc` 特性应用于枚举，每个枚举成员都会以枚举名称和成员名称拼接的形式暴露给 Objective-C 代码。成员名称的首字母会大写。例如，Swift `Planet` 枚举中名为 `venus` 的成员会作为名为 `PlanetVenus` 的成员暴露给 Objective-C 代码。

`objc` 特性可选地接受一个特性实参，该实参由一个标识符组成。该标识符为 `objc` 特性所适用的实体指定要暴露给 Objective-C 的名称。你可以使用此实参为类、枚举、枚举成员、协议、方法、getter、setter 和构造器命名。如果你为类、协议或枚举指定 Objective-C 名称，请在该名称中包含一个三字母前缀，如 [Programming with Objective-C](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Introduction/Introduction.html#//apple_ref/doc/uid/TP40011210) 中的 [Conventions](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Conventions/Conventions.html#//apple_ref/doc/uid/TP40011210-CH10-SW1) 所述。下面的示例把 `ExampleClass` 的 `enabled` 属性的 getter 以 `isEnabled` 而不是该属性本身的名称暴露给 Objective-C 代码。

```swift
class ExampleClass: NSObject {
    @objc var enabled: Bool {
        @objc(isEnabled) get {
            // Return the appropriate value
        }
    }
}
```

有关更多信息，请参阅 [Importing Swift into Objective-C](https://developer.apple.com/documentation/swift/imported_c_and_objective-c_apis/importing_swift_into_objective-c)。

> 注意：`objc` 特性的实参还可以更改该声明的运行时名称。在调用与 Objective-C 运行时交互的函数（例如 [`NSClassFromString(_:)`](https://developer.apple.com/documentation/foundation/nsclassfromstring(_:))）时，以及在应用的 Info.plist 文件中指定类名时，你会使用运行时名称。如果通过传递实参指定了名称，该名称会同时用作 Objective-C 代码中的名称和运行时名称。如果省略该实参，Objective-C 代码中使用的名称与 Swift 代码中的名称一致，而运行时名称遵循 Swift 编译器通常的名称重整约定。

### objcMembers {#objcMembers}

将此特性应用于类声明，以隐式地把 `objc` 特性应用于该类、其扩展、其子类以及其子类的所有扩展中所有与 Objective-C 兼容的成员。

大多数代码应改用 `objc` 特性，只暴露所需的声明。如果需要暴露许多声明，可以把它们分到一个带有 `objc` 特性的扩展中。`objcMembers` 特性是为大量使用 Objective-C 运行时内省设施的库提供的便利。在不需要时应用 `objc` 特性会增加二进制大小并对性能产生不利影响。

### preconcurrency {#preconcurrency}

将此特性应用于声明，以抑制严格的并发检查。你可以将此特性应用于以下类型的声明：

- 导入
- 结构体、类和 actor
- 枚举和枚举成员
- 协议
- 变量和常量
- 下标
- 构造器
- 函数

在导入声明上，此特性会降低使用所导入模块中类型的代码的并发检查严格程度。具体来说，所导入模块中未显式标记为不可发送的类型，可以在要求可发送类型的上下文中使用。

在其他声明上，此特性会降低使用所声明符号的代码的并发检查严格程度。当你在并发检查最少的范围内使用该符号时，不会检查该符号指定的与并发相关的约束，例如 `Sendable` 要求或全局 actor。

你可以按以下方式使用此特性，以帮助把代码迁移到严格并发检查：

1. 启用严格检查。
1. 对于尚未启用严格检查的模块，使用 `preconcurrency` 特性标注其导入。
1. 在把某个模块迁移到严格检查之后，移除 `preconcurrency` 特性。对于导入上的 `preconcurrency` 特性不再起作用且应被移除的任何位置，编译器都会发出警告。

对于其他声明，如果在向声明添加与并发相关的约束时仍有尚未迁移到严格检查的客户端，请添加 `preconcurrency` 特性。等所有客户端都迁移之后，再移除 `preconcurrency` 特性。

来自 Objective-C 的声明在导入时总是被视为标记了 `preconcurrency` 特性。

### propertyWrapper {#propertyWrapper}

将此特性应用于类、结构体或枚举声明，以将该类型用作属性包装器。当你把此特性应用于某个类型时，会创建一个与该类型同名的自定义特性。把新特性应用于类、结构体或枚举的属性，以通过包装器类型的实例来包装对该属性的访问；把该特性应用于局部存储变量声明，以同样的方式包装对该变量的访问。计算变量、全局变量和常量不能使用属性包装器。

包装器必须定义一个 `wrappedValue` 实例属性。属性的*被包装值*（wrapped value）是该属性的 getter 和 setter 所暴露的值。大多数情况下，`wrappedValue` 是计算值，但也可以是存储值。包装器定义并管理其被包装值所需的任何底层存储。编译器通过在被包装属性的名称前加下划线（`_`）来为包装器类型的实例合成存储——例如，`someProperty` 的包装器存储为 `_someProperty`。为包装器合成的存储具有 `private` 访问级别。

具有属性包装器的属性可以包含 `willSet` 和 `didSet` 块，但不能重写编译器合成的 `get` 或 `set` 块。

Swift 为属性包装器的初始化提供了两种语法糖。你可以在被包装值的定义中使用赋值语法，把赋值右侧的表达式作为实参传给属性包装器构造器的 `wrappedValue` 形参。也可以在把特性应用于属性时向该特性提供实参，这些实参会被传给属性包装器的构造器。例如，在下面的代码中，`SomeStruct` 调用了 `SomeWrapper` 定义的每个构造器。

```swift
@propertyWrapper
struct SomeWrapper {
    var wrappedValue: Int
    var someValue: Double
    init() {
        self.wrappedValue = 100
        self.someValue = 12.3
    }
    init(wrappedValue: Int) {
        self.wrappedValue = wrappedValue
        self.someValue = 45.6
    }
    init(wrappedValue value: Int, custom: Double) {
        self.wrappedValue = value
        self.someValue = custom
    }
}

struct SomeStruct {
    // Uses init()
    @SomeWrapper var a: Int

    // Uses init(wrappedValue:)
    @SomeWrapper var b = 10

    // Both use init(wrappedValue:custom:)
    @SomeWrapper(custom: 98.7) var c = 30
    @SomeWrapper(wrappedValue: 30, custom: 98.7) var d
}
```

被包装属性的*投射值*（projected value）是属性包装器可以用来暴露额外功能的第二个值。属性包装器类型的作者负责确定其投射值的含义，并定义该投射值所暴露的接口。要从属性包装器投射一个值，请在包装器类型上定义一个 `projectedValue` 实例属性。编译器通过在被包装属性名称前加美元符号（`$`）来为投射值合成标识符——例如，`someProperty` 的投射值是 `$someProperty`。投射值具有与原始被包装属性相同的访问级别。

```swift
@propertyWrapper
struct WrapperWithProjection {
    var wrappedValue: Int
    var projectedValue: SomeProjection {
        return SomeProjection(wrapper: self)
    }
}
struct SomeProjection {
    var wrapper: WrapperWithProjection
}

struct SomeStruct {
    @WrapperWithProjection var x = 123
}
let s = SomeStruct()
s.x           // Int value
s.$x          // SomeProjection value
s.$x.wrapper  // WrapperWithProjection value
```

如果投射值包装的变量使用原始标识符命名，那么你也要使用原始标识符来引用该值的合成存储及其投射值。请把下划线或美元符号写在反引号内；例如，`` someValue.`_some raw identifier` `` 指的是名称中包含空格的某个属性的合成存储，`` someValue.`$some raw identifier` `` 指的是它的投射值。

### resultBuilder {#resultBuilder}

将此特性应用于类、结构体或枚举，以将该类型用作结果构造器。*结果构造器*（result builder）是一种逐步构建嵌套数据结构的类型。你使用结果构造器来实现领域特定语言（DSL），以自然、声明式的方式创建嵌套数据结构。关于如何使用 `resultBuilder` 特性的示例，请参阅 [结果构造器](advanced-operators.md#Result-Builders)。

#### 结果构造方法 {#Result-Building-Methods}

结果构造器实现下面描述的静态方法。由于结果构造器的所有功能都通过静态方法暴露，你永远不需要初始化该类型的实例。结果构造器必须实现 `buildBlock(_:)` 方法，或者同时实现 `buildPartialBlock(first:)` 和 `buildPartialBlock(accumulated:next:)` 两个方法。其他方法——它们在 DSL 中启用额外功能——是可选的。结果构造器类型的声明实际上不必包含任何协议一致性。

静态方法的描述使用三个类型作为占位符。类型 `Expression` 是结果构造器输入类型的占位符，`Component` 是部分结果类型的占位符，`FinalResult` 是结果构造器所产生结果类型的占位符。你要把这些类型替换为结果构造器实际使用的类型。如果你的结果构造方法没有为 `Expression` 或 `FinalResult` 指定类型，它们默认与 `Component` 相同。

构建块的方法如下：

- **`static func buildBlock(_ components: Component...) -> Component`**：把部分结果组成的数组合并成一个部分结果。

- **`static func buildPartialBlock(first: Component) -> Component`**：从第一个组成部分构建一个部分结果组成部分。同时实现此方法和 `buildPartialBlock(accumulated:next:)`，以支持一次构建一个组成部分的块。与 `buildBlock(_:)` 相比，这种方法减少了对处理不同数量实参的泛型重载的需求。

- **`static func buildPartialBlock(accumulated: Component, next: Component) -> Component`**：通过把累积的组成部分与新的组成部分组合起来，构建一个部分结果组成部分。同时实现此方法和 `buildPartialBlock(first:)`，以支持一次构建一个组成部分的块。与 `buildBlock(_:)` 相比，这种方法减少了对处理不同数量实参的泛型重载的需求。

结果构造器可以实现上面列出的全部三个构建块的方法；在这种情况下，由可用性决定调用哪个方法。默认情况下，Swift 调用 `buildPartialBlock(first:)` 和 `buildPartialBlock(accumulated:next:)` 方法。要让 Swift 改为调用 `buildBlock(_:)`，请把外层声明的可用性标记为早于你在 `buildPartialBlock(first:)` 和 `buildPartialBlock(accumulated:next:)` 上写的可用性。

其他结果构造方法如下：

- **`static func buildOptional(_ component: Component?) -> Component`**：从可以为 `nil` 的部分结果构建一个部分结果。实现此方法以支持不包含 `else` 子句的 `if` 语句。

- **`static func buildEither(first: Component) -> Component`**：构建一个值取决于某个条件的部分结果。同时实现此方法和 `buildEither(second:)`，以支持 `switch` 语句和包含 `else` 子句的 `if` 语句。

- **`static func buildEither(second: Component) -> Component`**：构建一个值取决于某个条件的部分结果。同时实现此方法和 `buildEither(first:)`，以支持 `switch` 语句和包含 `else` 子句的 `if` 语句。

- **`static func buildArray(_ components: [Component]) -> Component`**：从部分结果组成的数组构建一个部分结果。实现此方法以支持 `for` 循环。

- **`static func buildExpression(_ expression: Expression) -> Component`**：从表达式构建一个部分结果。你可以实现此方法以执行预处理（例如把表达式转换为内部类型），或为使用位置处的类型推断提供附加信息。

- **`static func buildFinalResult(_ component: Component) -> FinalResult`**：从部分结果构建最终结果。如果结果构造器对部分结果和最终结果使用不同的类型，或者需要在返回结果之前对结果执行其他后处理，你可以实现此方法。

- **`static func buildLimitedAvailability(_ component: Component) -> Component`**：构建一个擦除类型信息的部分结果。你可以实现此方法，以防止类型信息传播到执行可用性检查的编译器控制语句之外。

例如，下面的代码定义了一个构建整数数组的简单结果构造器。这段代码把 `Component` 和 `Expression` 定义为类型别名，以便更容易地把下面的示例与上面的方法列表对应起来。

```swift
@resultBuilder
struct ArrayBuilder {
    typealias Component = [Int]
    typealias Expression = Int
    static func buildExpression(_ element: Expression) -> Component {
        return [element]
    }
    static func buildOptional(_ component: Component?) -> Component {
        guard let component = component else { return [] }
        return component
    }
    static func buildEither(first component: Component) -> Component {
        return component
    }
    static func buildEither(second component: Component) -> Component {
        return component
    }
    static func buildArray(_ components: [Component]) -> Component {
        return Array(components.joined())
    }
    static func buildBlock(_ components: Component...) -> Component {
        return Array(components.joined())
    }
}
```

#### 结果转换 {#Result-Transformations}

以下语法转换会递归应用，把使用结果构造器语法的代码转换为调用结果构造器类型静态方法的代码：

- 如果结果构造器有 `buildExpression(_:)` 方法，每个表达式都会变成对该方法的调用。此转换总是最先执行。例如，以下声明是等价的：

  ```swift
  @ArrayBuilder var builderNumber: [Int] { 10 }
  var manualNumber = ArrayBuilder.buildExpression(10)
  ```

- 赋值语句会像表达式一样被转换，但会被理解为求值为 `()`。你可以定义接受 `()` 类型实参的 `buildExpression(_:)` 重载，以专门处理赋值。
- 如果实现了 `buildLimitedAvailability(_:)` 方法，检查可用性条件的分支语句会变成对该方法的调用。如果没有实现 `buildLimitedAvailability(_:)`，那么检查可用性的分支语句会使用与其他分支语句相同的转换。此转换发生在转换为对 `buildEither(first:)`、`buildEither(second:)` 或 `buildOptional(_:)` 的调用之前。

你可以使用 `buildLimitedAvailability(_:)` 方法擦除根据所走分支而变化的类型信息。例如，下面的 `buildEither(first:)` 和 `buildEither(second:)` 方法使用一个捕获两个分支类型信息的泛型类型。

  ```swift
  protocol Drawable {
      func draw() -> String
  }
  struct Text: Drawable {
      var content: String
      init(_ content: String) { self.content = content }
      func draw() -> String { return content }
  }
  struct Line<D: Drawable>: Drawable {
      var elements: [D]
      func draw() -> String {
          return elements.map { $0.draw() }.joined(separator: "")
      }
  }
  struct DrawEither<First: Drawable, Second: Drawable>: Drawable {
      var content: Drawable
      func draw() -> String { return content.draw() }
  }

  @resultBuilder
  struct DrawingBuilder {
      static func buildBlock<D: Drawable>(_ components: D...) -> Line<D> {
          return Line(elements: components)
      }
      static func buildEither<First, Second>(first: First)
              -> DrawEither<First, Second> {
          return DrawEither(content: first)
      }
      static func buildEither<First, Second>(second: Second)
              -> DrawEither<First, Second> {
          return DrawEither(content: second)
      }
  }
  ```

然而，这种方法会在带可用性检查的代码中引发问题：

  ```swift
  @available(macOS 99, *)
  struct FutureText: Drawable {
      var content: String
      init(_ content: String) { self.content = content }
      func draw() -> String { return content }
  }
  @DrawingBuilder var brokenDrawing: Drawable {
      if #available(macOS 99, *) {
          FutureText("Inside.future")  // Problem
      } else {
          Text("Inside.present")
      }
  }
  // The type of brokenDrawing is Line<DrawEither<Line<FutureText>, Line<Text>>>
  ```

在上面的代码中，`FutureText` 作为 `brokenDrawing` 类型的一部分出现，因为它是 `DrawEither` 泛型类型中的类型之一。如果 `FutureText` 在运行时不可用，这可能会导致程序崩溃，即使在该类型被明确指定为不使用的情况下也是如此。

为了解决这个问题，实现一个 `buildLimitedAvailability(_:)` 方法，通过返回一个始终可用的类型来擦除类型信息。例如，下面的代码根据可用性检查构建一个 `AnyDrawable` 值。

  ```swift
  struct AnyDrawable: Drawable {
      var content: Drawable
      func draw() -> String { return content.draw() }
  }
  extension DrawingBuilder {
      static func buildLimitedAvailability(_ content: some Drawable) -> AnyDrawable {
          return AnyDrawable(content: content)
      }
  }

  @DrawingBuilder var typeErasedDrawing: Drawable {
      if #available(macOS 99, *) {
          FutureText("Inside.future")
      } else {
          Text("Inside.present")
      }
  }
  // The type of typeErasedDrawing is Line<DrawEither<AnyDrawable, Line<Text>>>
  ```

- 分支语句会变成一系列对 `buildEither(first:)` 和 `buildEither(second:)` 方法的嵌套调用。语句的条件和分支会被映射到一棵二叉树的叶节点上，而该语句会变成沿着从根节点到该叶节点的路径对 `buildEither` 方法的嵌套调用。

例如，如果你编写一个包含三个分支的 switch 语句，编译器会使用一棵有三个叶节点的二叉树。同样，由于从根节点到第二个分支的路径是“第二个子节点”然后“第一个子节点”，该分支会变成类似 `buildEither(first: buildEither(second: ... ))` 的嵌套调用。以下声明是等价的：

  ```swift
  let someNumber = 19
  @ArrayBuilder var builderConditional: [Int] {
      if someNumber < 12 {
          31
      } else if someNumber == 19 {
          32
      } else {
          33
      }
  }

  var manualConditional: [Int]
  if someNumber < 12 {
      let partialResult = ArrayBuilder.buildExpression(31)
      let outerPartialResult = ArrayBuilder.buildEither(first: partialResult)
      manualConditional = ArrayBuilder.buildEither(first: outerPartialResult)
  } else if someNumber == 19 {
      let partialResult = ArrayBuilder.buildExpression(32)
      let outerPartialResult = ArrayBuilder.buildEither(second: partialResult)
      manualConditional = ArrayBuilder.buildEither(first: outerPartialResult)
  } else {
      let partialResult = ArrayBuilder.buildExpression(33)
      manualConditional = ArrayBuilder.buildEither(second: partialResult)
  }
  ```

- 可能不产生值的分支语句（例如不带 `else` 子句的 `if` 语句）会变成对 `buildOptional(_:)` 的调用。如果 `if` 语句的条件成立，其代码块会被转换并作为实参传入；否则，以 `nil` 作为实参调用 `buildOptional(_:)`。例如，以下声明是等价的：

  ```swift
  @ArrayBuilder var builderOptional: [Int] {
      if (someNumber % 2) == 1 { 20 }
  }

  var partialResult: [Int]? = nil
  if (someNumber % 2) == 1 {
      partialResult = ArrayBuilder.buildExpression(20)
  }
  var manualOptional = ArrayBuilder.buildOptional(partialResult)
  ```

- 如果结果构造器实现了 `buildPartialBlock(first:)` 和 `buildPartialBlock(accumulated:next:)` 方法，代码块或 `do` 语句会变成对这些方法的调用。块内的第一条语句会被转换，作为 `buildPartialBlock(first:)` 方法的实参，其余语句则变成对 `buildPartialBlock(accumulated:next:)` 方法的嵌套调用。例如，以下声明是等价的：

  ```swift
  struct DrawBoth<First: Drawable, Second: Drawable>: Drawable {
      var first: First
      var second: Second
      func draw() -> String { return first.draw() + second.draw() }
  }

  @resultBuilder
  struct DrawingPartialBlockBuilder {
      static func buildPartialBlock<D: Drawable>(first: D) -> D {
          return first
      }
      static func buildPartialBlock<Accumulated: Drawable, Next: Drawable>(
          accumulated: Accumulated, next: Next
      ) -> DrawBoth<Accumulated, Next> {
          return DrawBoth(first: accumulated, second: next)
      }
  }

  @DrawingPartialBlockBuilder var builderBlock: some Drawable {
      Text("First")
      Line(elements: [Text("Second"), Text("Third")])
      Text("Last")
  }

  let partialResult1 = DrawingPartialBlockBuilder.buildPartialBlock(first: Text("first"))
  let partialResult2 = DrawingPartialBlockBuilder.buildPartialBlock(
      accumulated: partialResult1,
      next: Line(elements: [Text("Second"), Text("Third")])
  )
  let manualResult = DrawingPartialBlockBuilder.buildPartialBlock(
      accumulated: partialResult2,
      next: Text("Last")
  )
  ```

- 否则，代码块或 `do` 语句会变成对 `buildBlock(_:)` 方法的调用。块内的每条语句会逐一被转换，并成为 `buildBlock(_:)` 方法的实参。例如，以下声明是等价的：

  ```swift
  @ArrayBuilder var builderBlock: [Int] {
      100
      200
      300
  }

  var manualBlock = ArrayBuilder.buildBlock(
      ArrayBuilder.buildExpression(100),
      ArrayBuilder.buildExpression(200),
      ArrayBuilder.buildExpression(300)
  )
  ```

- `for` 循环会变成一个临时变量、一个 `for` 循环以及对 `buildArray(_:)` 方法的调用。新的 `for` 循环遍历序列，并把每个部分结果追加到该数组中。临时数组作为实参传入 `buildArray(_:)` 调用。例如，以下声明是等价的：

  ```swift
  @ArrayBuilder var builderArray: [Int] {
      for i in 5...7 {
          100 + i
      }
  }

  var temporary: [[Int]] = []
  for i in 5...7 {
      let partialResult = ArrayBuilder.buildExpression(100 + i)
      temporary.append(partialResult)
  }
  let manualArray = ArrayBuilder.buildArray(temporary)
  ```

- 如果结果构造器有 `buildFinalResult(_:)` 方法，最终结果会变成对该方法的调用。此转换总是最后执行。

尽管转换行为是以临时变量的方式来描述的，但使用结果构造器实际上不会创建任何从代码其余部分可见的新声明。

在结果构造器转换的代码中，不能使用 `break`、`continue`、`defer`、`guard` 或 `return` 语句，也不能使用 `while` 语句或 `do`-`catch` 语句。

转换过程不会更改代码中的声明，这让你可以使用临时常量和变量逐步构建表达式。它也不会更改 `throw` 语句、编译时诊断语句或包含 `return` 语句的闭包。

只要可能，转换就会合并。例如，表达式 `4 + 5 * 6` 会变成 `buildExpression(4 + 5 * 6)`，而不是对该函数的多次调用。同样，嵌套的分支语句会变成一棵由对 `buildEither` 方法的调用组成的单一二叉树。

#### 自定义结果构造器特性 {#Custom-Result-Builder-Attributes}

创建结果构造器类型会创建一个同名的自定义特性。你可以在以下位置应用该特性：

- 在函数声明上，结果构造器构建函数体。
- 在包含 getter 的变量或下标声明上，结果构造器构建 getter 的主体。
- 在函数声明的形参上，结果构造器构建作为对应实参传入的闭包的主体。

应用结果构造器特性不会影响 ABI 兼容性。把结果构造器特性应用于形参会使该特性成为函数接口的一部分，这可能会影响源码兼容性。

### requires_stored_property_inits {#requiresstoredpropertyinits}

将此特性应用于类声明，以要求该类中的所有存储属性都在其定义中提供默认值。对于任何继承自 `NSManagedObject` 的类，都会推断出此特性。

### testable {#testable}

将此特性应用于 `import` 声明，以便在导入该模块时更改其访问控制，从而简化该模块代码的测试。所导入模块中标记了 `internal` 访问级别修饰符的实体会被视为使用 `public` 访问级别修饰符声明的那样导入。标记了 `internal` 或 `public` 访问级别修饰符的类和类成员会被视为使用 `open` 访问级别修饰符声明的那样导入。所导入的模块必须在启用测试的情况下编译。

### UIApplicationMain {#UIApplicationMain}

> 已弃用：此特性已弃用；请改用 [main](attributes.md#main) 特性。在 Swift 6 中，使用此特性会产生编译时错误。

将此特性应用于类，以表示它是应用委托。使用此特性等同于调用 `UIApplicationMain` 函数并传入该类的名称作为委托类的名称。

如果不使用此特性，请提供一个 `main.swift` 文件，并在顶层包含调用 [`UIApplicationMain(_:_:_:_:)`](https://developer.apple.com/documentation/uikit/uiapplicationmain(_:_:_:_:)-1yub7) 函数的代码。例如，如果你的应用使用 `UIApplication` 的自定义子类作为其主类，请调用 `UIApplicationMain(_:_:_:_:)` 函数，而不是使用此特性。

你编译为可执行文件的 Swift 代码最多只能包含一个顶层入口点，如 [顶层代码](declarations.md#Top-Level-Code) 中所述。

### unchecked {#unchecked}

将此特性作为类型声明所采纳协议列表的一部分应用于协议类型，以关闭对该协议要求的强制实施。

唯一支持的协议是 [`Sendable`](https://developer.apple.com/documentation/swift/sendable)。

### usableFromInline {#usableFromInline}

将此特性应用于函数、方法、计算属性、下标、构造器或析构器声明，以允许在与该声明相同的模块中定义的可内联代码使用该符号。该声明必须具有 `internal` 访问级别修饰符。标记为 `usableFromInline` 的结构体或类，其属性只能使用 `public` 或 `usableFromInline` 的类型。标记为 `usableFromInline` 的枚举，其成员的原始值和关联值只能使用 `public` 或 `usableFromInline` 的类型。

与 `public` 访问级别修饰符一样，此特性会把声明作为模块公开接口的一部分暴露出来。与 `public` 不同的是，即使声明的符号已导出，编译器也不允许模块外部的代码按名称引用标记了 `usableFromInline` 的声明。不过，模块外部的代码仍可能通过运行时行为与该声明的符号交互。

标记了 `inlinable` 特性的声明隐式地可由可内联代码使用。虽然 `inlinable` 或 `usableFromInline` 都可以应用于 `internal` 声明，但同时应用这两个特性是错误的。

### warn_unqualified_access {#warnunqualifiedaccess}

将此特性应用于顶层函数、实例方法，或者类方法或静态方法，以便在该函数或方法没有前置限定符（例如模块名、类型名或实例变量或常量）而被使用时触发警告。使用此特性有助于避免从同一作用域可访问的同名函数之间产生歧义。

例如，Swift 标准库既包含顶层 [`min(_:_:)`](https://developer.apple.com/documentation/swift/min(_:_:)) 函数，也包含用于元素可比较的序列的 [`min()`](https://developer.apple.com/documentation/swift/sequence/min()) 方法。序列方法使用 `warn_unqualified_access` 特性声明，以帮助减少在 `Sequence` 扩展中尝试使用其中一个时的困惑。

### Interface Builder 使用的声明特性 {#Declaration-Attributes-Used-by-Interface-Builder}

Interface Builder 特性是 Interface Builder 用来与 Xcode 同步的声明特性。Swift 提供以下 Interface Builder 特性：`IBAction`、`IBSegueAction`、`IBOutlet`、`IBDesignable` 和 `IBInspectable`。这些特性在概念上与它们在 Objective-C 中的对应物相同。

你把 `IBOutlet` 和 `IBInspectable` 特性应用于类的属性声明。你把 `IBAction` 和 `IBSegueAction` 特性应用于类的方法声明，把 `IBDesignable` 特性应用于类声明。

应用 `IBAction`、`IBSegueAction`、`IBOutlet`、`IBDesignable` 或 `IBInspectable` 特性还隐含 `objc` 特性。

## 类型特性 {#Type-Attributes}

类型特性只能应用于类型。

### autoclosure {#autoclosure}

应用此特性可以通过把表达式自动包装在不接受实参的闭包中，来延迟该表达式的求值。你把它应用于函数或方法声明中形参的类型，该形参的类型是一个不接受实参并返回表达式类型值的函数类型。关于如何使用 `autoclosure` 特性的示例，请参阅 [自动闭包](closures.md#Autoclosures) 和 [函数类型](types.md#Function-Type)。

### convention {#convention}

将此特性应用于函数的类型，以指明其调用约定。

`convention` 特性总是与以下实参之一一起出现：

- `swift` 实参指明 Swift 函数引用。这是 Swift 中函数值的标准调用约定。
- `block` 实参指明与 Objective-C 兼容的 block 引用。函数值表示为对 block 对象的引用，该对象是一个与 `id` 兼容的 Objective-C 对象，其调用函数内嵌在该对象中。调用函数使用 C 调用约定。
- `c` 实参指明 C 函数引用。该函数值不携带上下文，并使用 C 调用约定。

除少数例外，当需要具有某种调用约定的函数时，可以使用任何调用约定的函数。非泛型全局函数、不捕获任何局部变量的局部函数，或不捕获任何局部变量的闭包，可以转换为 C 调用约定。其他 Swift 函数不能转换为 C 调用约定。具有 Objective-C block 调用约定的函数不能转换为 C 调用约定。

### escaping {#escaping}

将此特性应用于函数或方法声明中形参的类型，以表示该形参的值可以被存储起来供以后执行。这意味着该值的存活期允许超过调用的生命周期。带有 `escaping` 类型特性的函数类型形参要求对属性或方法显式使用 `self.`。关于如何使用 `escaping` 特性的示例，请参阅 [逃逸闭包](closures.md#Escaping-Closures)。

### Sendable {#Sendable}

将此特性应用于函数的类型，以表示该函数或闭包是可发送的。将此特性应用于函数类型，与让非函数类型遵循 [`Sendable`](https://developer.apple.com/documentation/swift/sendable) 协议含义相同。

如果函数或闭包用在期望可发送值的上下文中，并且该函数或闭包满足可发送的要求，那么会在该函数或闭包上推断出此特性。

可发送函数类型是对应的不可发送函数类型的子类型。

## Switch 分支特性 {#Switch-Case-Attributes}

Switch 分支特性只能应用于 switch 分支。

### unknown {#unknown}

将此特性应用于 switch 分支，以表示在代码编译时已知的枚举成员中，预计不会有任何成员匹配该分支。关于如何使用 `unknown` 特性的示例，请参阅 [对将来新增的枚举成员进行切换](statements.md#Switching-Over-Future-Enumeration-Cases)。

> 特性语法：
>
> *attribute* → **`@`** *attribute-name* *attribute-argument-clause*_?_ \
> *attribute-name* → *identifier* \
> *attribute-argument-clause* → **`(`** *balanced-tokens*_?_ **`)`** \
> *attributes* → *attribute* *attributes*_?_
>
> *balanced-tokens* → *balanced-token* *balanced-tokens*_?_ \
> *balanced-token* → **`(`** *balanced-tokens*_?_ **`)`** \
> *balanced-token* → **`[`** *balanced-tokens*_?_ **`]`** \
> *balanced-token* → **`{`** *balanced-tokens*_?_ **`}`** \
> *balanced-token* → Any identifier, keyword, literal, or operator \
> *balanced-token* → Any punctuation except  **`(`**,  **`)`**,  **`[`**,  **`]`**,  **`{`**, or  **`}`**
