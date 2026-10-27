# 可选链 {#Optional-Chaining}

无需解包即可访问可选值的成员。

*可选链*（optional chaining）是一种在当前可能为 `nil` 的可选值上查询和调用属性、方法及下标的过程。如果可选值包含值，属性、方法或下标调用就会成功；如果可选值是 `nil`，属性、方法或下标调用就返回 `nil`。多个查询可以链接在一起，而且只要链条中任一环节是 `nil`，整个链条就会优雅地失败。

> 注意：Swift 中的可选链类似于 Objective-C 中向 `nil` 发送消息，但它的方式适用于任何类型，并且可以检查成功还是失败。

## 可选链作为强制解包的替代方案 {#Optional-Chaining-as-an-Alternative-to-Forced-Unwrapping}

你通过在可选值后面放置一个问号（`?`）来指定可选链；如果该可选值非 `nil`，你就想在这个可选值上调用属性、方法或下标。这与在可选值后面放一个感叹号（`!`）来强制解包其值非常相似。主要区别在于：可选值为 `nil` 时，可选链会优雅地失败；而强制解包在可选值为 `nil` 时会触发运行时错误。

为了体现可选链可以在 `nil` 值上调用这一事实，可选链调用的结果始终是可选值，即使你查询的属性、方法或下标返回的是非可选值。你可以用这个可选返回值来检查可选链调用是否成功（返回的可选值包含值），或者是否因为链条中存在 `nil` 值而失败（返回的可选值是 `nil`）。

具体来说，可选链调用的结果与预期的返回值类型相同，但被包装在可选值中。通常返回 `Int` 的属性，通过可选链访问时会返回 `Int?`。

接下来的几段代码演示了可选链与强制解包的区别，以及它如何让你检查调用是否成功。

首先，定义两个分别名为 `Person` 和 `Residence` 的类：

```swift
class Person {
    var residence: Residence?
}

class Residence {
    var numberOfRooms = 1
}
```

`Residence` 实例有一个名为 `numberOfRooms` 的 `Int` 属性，默认值为 `1`。`Person` 实例有一个类型为 `Residence?` 的可选属性 `residence`。

如果你创建一个新的 `Person` 实例，它的 `residence` 属性会因为可选而默认初始化为 `nil`。在下面的代码中，`john` 的 `residence` 属性值为 `nil`：

```swift
let john = Person()
```

如果你试图访问这个人的 `residence` 的 `numberOfRooms` 属性，在 `residence` 后面加感叹号强制解包其值，就会触发运行时错误，因为根本没有可解包的 `residence` 值：

```swift
let roomCount = john.residence!.numberOfRooms
// this triggers a runtime error
```

当 `john.residence` 有非 `nil` 值时，上面的代码会成功，并把 `roomCount` 设为一个包含相应房间数的 `Int` 值。然而，如上所示，当 `residence` 是 `nil` 时，这段代码总会触发运行时错误。

可选链提供了访问 `numberOfRooms` 值的另一种方式。要使用可选链，用问号代替感叹号：

```swift
if let roomCount = john.residence?.numberOfRooms {
    print("John's residence has \(roomCount) room(s).")
} else {
    print("Unable to retrieve the number of rooms.")
}
// Prints "Unable to retrieve the number of rooms."
```

这段代码告诉 Swift 在可选的 `residence` 属性上进行“链接”，如果 `residence` 存在，就取出 `numberOfRooms` 的值。

因为访问 `numberOfRooms` 的尝试可能失败，所以这次可选链尝试返回一个 `Int?` 类型的值，即“可选 `Int`”。当 `residence` 为 `nil` 时，如上面的例子所示，这个可选 `Int` 也会是 `nil`，以体现无法访问 `numberOfRooms` 这一事实。通过可选绑定访问这个可选 `Int`，解包其中的整数，并把非可选值赋给常量 `roomCount`。

请注意，即使 `numberOfRooms` 是非可选的 `Int`，情况也是如此。通过可选链查询它，意味着对 `numberOfRooms` 的调用始终返回 `Int?` 而不是 `Int`。

你可以把一个 `Residence` 实例赋给 `john.residence`，使它不再是 `nil`：

```swift
john.residence = Residence()
```

`john.residence` 现在包含一个实际的 `Residence` 实例，而不是 `nil`。如果你像之前一样用相同的可选链访问 `numberOfRooms`，它现在会返回一个包含 `numberOfRooms` 默认值 `1` 的 `Int?`：

```swift
if let roomCount = john.residence?.numberOfRooms {
    print("John's residence has \(roomCount) room(s).")
} else {
    print("Unable to retrieve the number of rooms.")
}
// Prints "John's residence has 1 room(s)."
```

## 为可选链定义模型类 {#Defining-Model-Classes-for-Optional-Chaining}

你可以把可选链用于层级不止一层的属性、方法和下标调用。这让你能够深入相互关联的复杂类型模型中的子属性，并检查是否可以在这些子属性上访问属性、方法和下标。

下面的代码定义了四个模型类，供后面的几个示例使用，包括多级可选链的示例。这些类在之前的 `Person` 和 `Residence` 模型基础上扩展，增加了 `Room` 和 `Address` 类，以及相关的属性、方法和下标。

`Person` 类的定义与之前相同：

```swift
class Person {
    var residence: Residence?
}
```

`Residence` 类比之前更复杂。这次 `Residence` 类定义了一个名为 `rooms` 的变量属性，它用一个类型为 `[Room]` 的空数组初始化：

```swift
class Residence {
    var rooms: [Room] = []
    var numberOfRooms: Int {
        return rooms.count
    }
    subscript(i: Int) -> Room {
        get {
            return rooms[i]
        }
        set {
            rooms[i] = newValue
        }
    }
    func printNumberOfRooms() {
        print("The number of rooms is \(numberOfRooms)")
    }
    var address: Address?
}
```

因为这个版本的 `Residence` 存储 `Room` 实例的数组，所以它的 `numberOfRooms` 属性实现为计算属性，而不是存储属性。计算属性 `numberOfRooms` 只是返回 `rooms` 数组的 `count` 属性的值。

作为访问 `rooms` 数组的快捷方式，这个版本的 `Residence` 提供了一个读写下标，用于访问 `rooms` 数组中指定索引处的房间。

这个版本的 `Residence` 还提供了一个名为 `printNumberOfRooms` 的方法，它只是打印住所中的房间数。

最后，`Residence` 定义了一个名为 `address` 的可选属性，类型为 `Address?`。这个属性的 `Address` 类类型定义如下。

用于 `rooms` 数组的 `Room` 类是一个简单的类，有一个名为 `name` 的属性，以及一个把该属性设为某个合适房间名的构造器：

```swift
class Room {
    let name: String
    init(name: String) { self.name = name }
}
```

这个模型中的最后一个类名为 `Address`。这个类有三个类型为 `String?` 的可选属性。前两个属性 `buildingName` 和 `buildingNumber` 是标识地址中特定建筑物的两种替代方式。第三个属性 `street` 用于给出该地址所在的街道名：

```swift
class Address {
    var buildingName: String?
    var buildingNumber: String?
    var street: String?
    func buildingIdentifier() -> String? {
        if let buildingNumber = buildingNumber, let street = street {
            return "\(buildingNumber) \(street)"
        } else if buildingName != nil {
            return buildingName
        } else {
            return nil
        }
    }
}
```

`Address` 类还提供了一个名为 `buildingIdentifier()` 的方法，返回类型为 `String?`。这个方法检查地址的各属性：如果 `buildingName` 有值就返回它；如果 `buildingNumber` 和 `street` 都有值就返回二者拼接的结果；否则返回 `nil`。

## 通过可选链访问属性 {#Accessing-Properties-Through-Optional-Chaining}

如 [可选链作为强制解包的替代方案](optional-chaining.md#Optional-Chaining-as-an-Alternative-to-Forced-Unwrapping) 所示，你可以用可选链访问可选值上的属性，并检查该属性访问是否成功。

用上面定义的类创建一个新的 `Person` 实例，并像之前一样尝试访问它的 `numberOfRooms` 属性：

```swift
let john = Person()
if let roomCount = john.residence?.numberOfRooms {
    print("John's residence has \(roomCount) room(s).")
} else {
    print("Unable to retrieve the number of rooms.")
}
// Prints "Unable to retrieve the number of rooms."
```

因为 `john.residence` 是 `nil`，这次可选链调用和之前一样失败。

你也可以尝试通过可选链设置属性的值：

```swift
let someAddress = Address()
someAddress.buildingNumber = "29"
someAddress.street = "Acacia Road"
john.residence?.address = someAddress
```

在这个例子中，设置 `john.residence` 的 `address` 属性的尝试会失败，因为 `john.residence` 当前是 `nil`。

赋值是可选链的一部分，这意味着 `=` 运算符右侧的代码都不会被求值。在上一个例子中，不太容易看出 `someAddress` 从未被求值，因为访问常量没有任何副作用。下面的代码做同样的赋值，但用一个函数来创建地址。这个函数在返回值之前会打印“Function was called”，让你看出 `=` 运算符右侧是否被求值。

```swift
func createAddress() -> Address {
    print("Function was called.")

    let someAddress = Address()
    someAddress.buildingNumber = "29"
    someAddress.street = "Acacia Road"

    return someAddress
}
john.residence?.address = createAddress()
```

你可以判断出 `createAddress()` 函数没有被调用，因为什么都没打印。

## 通过可选链调用方法 {#Calling-Methods-Through-Optional-Chaining}

你可以用可选链在可选值上调用方法，并检查该方法调用是否成功。即使方法没有定义返回值，也可以这样做。

`Residence` 类上的 `printNumberOfRooms()` 方法打印 `numberOfRooms` 的当前值。这个方法如下：

```swift
func printNumberOfRooms() {
    print("The number of rooms is \(numberOfRooms)")
}
```

这个方法没有指定返回类型。不过，没有返回类型的函数和方法有一个隐式的 `Void` 返回类型，详见 [无返回值的函数](functions.md#Functions-Without-Return-Values)。这意味着它们返回 `()` 值，也就是空元组。

如果你用可选链在可选值上调用这个方法，方法的返回类型会是 `Void?` 而不是 `Void`，因为通过可选链调用时返回值总是可选类型。这让你可以用 `if` 语句检查是否成功调用了 `printNumberOfRooms()` 方法，即使该方法本身没有定义返回值。把 `printNumberOfRooms` 调用的返回值与 `nil` 比较，看方法调用是否成功：

```swift
if john.residence?.printNumberOfRooms() != nil {
    print("It was possible to print the number of rooms.")
} else {
    print("It was not possible to print the number of rooms.")
}
// Prints "It was not possible to print the number of rooms."
```

通过可选链设置属性时也是如此。上面 [通过可选链访问属性](optional-chaining.md#Accessing-Properties-Through-Optional-Chaining) 中的例子尝试为 `john.residence` 设置 `address` 值，即使 `residence` 属性是 `nil`。通过可选链设置属性的任何尝试都返回一个 `Void?` 类型的值，这让你可以与 `nil` 比较，看属性是否设置成功：

```swift
if (john.residence?.address = someAddress) != nil {
    print("It was possible to set the address.")
} else {
    print("It was not possible to set the address.")
}
// Prints "It was not possible to set the address."
```

## 通过可选链访问下标 {#Accessing-Subscripts-Through-Optional-Chaining}

你可以用可选链尝试从可选值的下标读取和设置值，并检查该下标调用是否成功。

> 注意：当你通过可选链访问可选值上的下标时，问号放在下标方括号的*前面*，而不是后面。可选链的问号总是紧跟在表达式中可选的那部分之后。

下面的例子使用 `Residence` 类上定义的下标，尝试读取 `john.residence` 属性的 `rooms` 数组中第一个房间的名称。因为 `john.residence` 当前是 `nil`，下标调用失败：

```swift
if let firstRoomName = john.residence?[0].name {
    print("The first room name is \(firstRoomName).")
} else {
    print("Unable to retrieve the first room name.")
}
// Prints "Unable to retrieve the first room name."
```

这个下标调用中的可选链问号紧跟在 `john.residence` 之后、下标方括号之前，因为 `john.residence` 是尝试应用可选链的那个可选值。

同样，你可以尝试通过可选链用下标设置新值：

```swift
john.residence?[0] = Room(name: "Bathroom")
```

这次下标设置尝试也会失败，因为 `residence` 当前是 `nil`。

如果你创建一个实际的 `Residence` 实例并把它赋给 `john.residence`，其 `rooms` 数组中有一个或多个 `Room` 实例，你就可以通过可选链用 `Residence` 下标访问 `rooms` 数组中的实际元素：

```swift
let johnsHouse = Residence()
johnsHouse.rooms.append(Room(name: "Living Room"))
johnsHouse.rooms.append(Room(name: "Kitchen"))
john.residence = johnsHouse

if let firstRoomName = john.residence?[0].name {
    print("The first room name is \(firstRoomName).")
} else {
    print("Unable to retrieve the first room name.")
}
// Prints "The first room name is Living Room."
```

### 访问可选类型的下标 {#Accessing-Subscripts-of-Optional-Type}

如果下标返回可选类型的值——例如 Swift `Dictionary` 类型的键下标——把问号放在下标右方括号*之后*，以便在其可选返回值上继续链接：

```swift
var testScores = ["Dave": [86, 82, 84], "Bev": [79, 94, 81]]
testScores["Dave"]?[0] = 91
testScores["Bev"]?[0] += 1
testScores["Brian"]?[0] = 72
// the "Dave" array is now [91, 82, 84] and the "Bev" array is now [80, 94, 81]
```

上面的例子定义了一个名为 `testScores` 的字典，它包含两个键值对，把 `String` 键映射到 `Int` 值的数组。这个例子用可选链把 `"Dave"` 数组的第一项设为 `91`；把 `"Bev"` 数组的第一项增加 `1`；并尝试为 `"Brian"` 键对应的数组设置第一项。前两次调用成功，因为 `testScores` 字典包含 `"Dave"` 和 `"Bev"` 键。第三次调用失败，因为 `testScores` 字典不包含 `"Brian"` 键。

## 链接多级可选链 {#Linking-Multiple-Levels-of-Chaining}

你可以把多级可选链链接在一起，深入模型内部，访问更深层的属性、方法和下标。不过，多级可选链不会给返回值增加更多层可选性。

换句话说：

- 如果你试图读取的类型不是可选类型，它会因为可选链而变成可选类型。
- 如果你试图读取的类型*已经是*可选类型，它不会因为链接而变得*更加*可选。

因此：

- 如果你试图通过可选链读取一个 `Int` 值，无论使用多少级可选链，返回的始终是 `Int?`。
- 同理，如果你试图通过可选链读取一个 `Int?` 值，无论使用多少级可选链，返回的始终是 `Int?`。

下面的例子尝试访问 `john` 的 `residence` 属性的 `address` 属性的 `street` 属性。这里使用了*两级*可选链，分别链接 `residence` 和 `address` 属性，二者都是可选类型：

```swift
if let johnsStreet = john.residence?.address?.street {
    print("John's street name is \(johnsStreet).")
} else {
    print("Unable to retrieve the address.")
}
// Prints "Unable to retrieve the address."
```

`john.residence` 的值当前包含一个有效的 `Residence` 实例。不过，`john.residence.address` 的值当前是 `nil`。因此，对 `john.residence?.address?.street` 的调用失败。

注意，在上面的例子中，你试图读取的是 `street` 属性的值。这个属性的类型是 `String?`。因此 `john.residence?.address?.street` 的返回值也是 `String?`，即使在该属性底层的可选类型之上还应用了两级可选链。

如果你把一个实际的 `Address` 实例设为 `john.residence.address` 的值，并为该地址的 `street` 属性设置实际值，就可以通过多级可选链访问 `street` 属性的值：

```swift
let johnsAddress = Address()
johnsAddress.buildingName = "The Larches"
johnsAddress.street = "Laurel Street"
john.residence?.address = johnsAddress

if let johnsStreet = john.residence?.address?.street {
    print("John's street name is \(johnsStreet).")
} else {
    print("Unable to retrieve the address.")
}
// Prints "John's street name is Laurel Street."
```

在这个例子中，设置 `john.residence` 的 `address` 属性的尝试会成功，因为 `john.residence` 的值当前包含一个有效的 `Residence` 实例。

## 在返回可选值的方法上继续链接 {#Chaining-on-Methods-with-Optional-Return-Values}

上一个例子展示了如何通过可选链读取可选类型属性的值。你也可以用可选链调用返回可选类型值的方法，并在需要时在该方法的返回值上继续链接。

下面的例子通过可选链调用 `Address` 类的 `buildingIdentifier()` 方法。这个方法返回 `String?` 类型的值。如上所述，可选链之后该方法调用的最终返回类型也是 `String?`：

```swift
if let buildingIdentifier = john.residence?.address?.buildingIdentifier() {
    print("John's building identifier is \(buildingIdentifier).")
}
// Prints "John's building identifier is The Larches."
```

如果你想在这个方法的返回值上执行进一步的可选链，把可选链问号放在方法括号*之后*：

```swift
if let beginsWithThe =
    john.residence?.address?.buildingIdentifier()?.hasPrefix("The") {
    if beginsWithThe {
        print("John's building identifier begins with \"The\".")
    } else {
        print("John's building identifier doesn't begin with \"The\".")
    }
}
// Prints "John's building identifier begins with "The"."
```

> 注意：在上面的例子中，你把可选链问号放在括号*之后*，因为你所链接的可选值是 `buildingIdentifier()` 方法的返回值，而不是 `buildingIdentifier()` 方法本身。
