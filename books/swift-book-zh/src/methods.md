# 方法 {#Methods}

定义并调用作为实例或类型一部分的函数。

*方法*是与特定类型关联的函数。类、结构体和枚举都可以定义实例方法，实例方法封装了处理某个类型实例的特定任务和功能。类、结构体和枚举也可以定义类型方法，类型方法与类型本身关联。类型方法类似于 Objective-C 中的类方法。

Swift 中的结构体和枚举可以定义方法，这是它与 C 和 Objective-C 的一大区别。在 Objective-C 中，只有类才能定义方法。在 Swift 中，你可以选择定义类、结构体还是枚举，同时仍然可以灵活地在你创建的类型上定义方法。

## 实例方法 {#Instance-Methods}

*实例方法*是属于特定类、结构体或枚举实例的函数。它们为这些实例的功能提供支持：既可以提供访问和修改实例属性的方式，也可以提供与该实例用途相关的功能。实例方法的语法与函数完全相同，详见 [函数](functions.md)。

实例方法写在所属类型的左花括号与右花括号之间。实例方法可以隐式访问该类型的所有其他实例方法和属性。实例方法只能在所属类型的特定实例上调用。没有现成实例时，无法单独调用它。

下面是一个例子，定义了一个简单的 `Counter` 类，用于统计某个动作发生的次数：

```swift
class Counter {
    var count = 0
    func increment() {
        count += 1
    }
    func increment(by amount: Int) {
        count += amount
    }
    func reset() {
        count = 0
    }
}
```

`Counter` 类定义了三个实例方法：

- `increment()` 把计数器增加 `1`。
- `increment(by: Int)` 把计数器增加指定的整数。
- `reset()` 把计数器重置为零。

`Counter` 类还声明了一个变量属性 `count`，用来记录当前计数器值。

调用实例方法所用的点语法与访问属性相同：

```swift
let counter = Counter()
// the initial counter value is 0
counter.increment()
// the counter's value is now 1
counter.increment(by: 5)
// the counter's value is now 6
counter.reset()
// the counter's value is now 0
```

函数的形参可以同时有一个名字（在函数体内部使用）和一个实参标签（调用函数时使用），详见 [函数实参标签和形参名](functions.md#Function-Argument-Labels-and-Parameter-Names)。方法形参也是如此，因为方法不过就是与类型关联的函数。

### self 属性 {#The-self-Property}

每个类型的实例都有一个名为 `self` 的隐式属性，它完全等同于实例本身。你在实例自己的实例方法中使用 `self` 属性来引用当前实例。

上面示例中的 `increment()` 方法也可以写成这样：

```swift
func increment() {
    self.count += 1
}
```

实际上，你在代码中并不需要经常写 `self`。如果你没有显式写出 `self`，那么当你在方法内使用已知的属性名或方法名时，Swift 会认为你指的是当前实例的属性或方法。`Counter` 的三个实例方法中使用 `count`（而不是 `self.count`）就体现了这一假设。

这条规则的主要例外是：实例方法的形参名与该实例的某个属性同名。在这种情况下，形参名优先，你必须用更明确的方式来引用属性。你可以用 `self` 属性来区分形参名和属性名。

在这里，`self` 用于区分名为 `x` 的方法形参和同样名为 `x` 的实例属性：

```swift
struct Point {
    var x = 0.0, y = 0.0
    func isToTheRightOf(x: Double) -> Bool {
        return self.x > x
    }
}
let somePoint = Point(x: 4.0, y: 5.0)
if somePoint.isToTheRightOf(x: 1.0) {
    print("This point is to the right of the line where x == 1.0")
}
// Prints "This point is to the right of the line where x == 1.0".
```

如果没有 `self` 前缀，Swift 会认为两处 `x` 指的都是名为 `x` 的方法形参。

### 在实例方法中修改值类型 {#Modifying-Value-Types-from-Within-Instance-Methods}

结构体和枚举是*值类型*。默认情况下，值类型的属性不能在其实例方法中修改。

不过，如果你需要在某个特定方法中修改结构体或枚举的属性，可以为该方法启用*可变*（mutating）行为。这样，该方法就可以在方法内部改变（也就是修改）其属性，所做的任何更改都会在方法结束时写回原始结构体。该方法还可以给隐式的 `self` 属性赋予一个全新的实例，这个新实例会在方法结束时替换现有实例。

要启用这种行为，请把 `mutating` 关键字放在该方法的 `func` 关键字之前：

```swift
struct Point {
    var x = 0.0, y = 0.0
    mutating func moveBy(x deltaX: Double, y deltaY: Double) {
        x += deltaX
        y += deltaY
    }
}
var somePoint = Point(x: 1.0, y: 1.0)
somePoint.moveBy(x: 2.0, y: 3.0)
print("The point is now at (\(somePoint.x), \(somePoint.y))")
// Prints "The point is now at (3.0, 4.0)".
```

上面的 `Point` 结构体定义了一个可变的 `moveBy(x:y:)` 方法，它把 `Point` 实例移动一定的距离。这个方法并不返回新的点，而是实际修改调用它的那个点。在定义中加入 `mutating` 关键字，是为了让它能够修改自身的属性。

注意，不能对结构体类型的常量调用可变方法，因为它的属性无法更改，即使这些属性是变量属性也不行，详见 [常量结构体实例的存储属性](properties.md#Stored-Properties-of-Constant-Structure-Instances)：

```swift
let fixedPoint = Point(x: 3.0, y: 3.0)
fixedPoint.moveBy(x: 2.0, y: 3.0)
// this will report an error
```

### 在可变方法中给 self 赋值 {#Assigning-to-self-Within-a-Mutating-Method}

可变方法可以给隐式的 `self` 属性赋予一个全新的实例。上面的 `Point` 示例也可以改写成下面这样：

```swift
struct Point {
    var x = 0.0, y = 0.0
    mutating func moveBy(x deltaX: Double, y deltaY: Double) {
        self = Point(x: x + deltaX, y: y + deltaY)
    }
}
```

这个版本的可变 `moveBy(x:y:)` 方法会创建一个新结构体，其 `x` 和 `y` 值被设为目标位置。调用这个替代版本的方法所得到的最终结果，与调用前面那个版本完全相同。

枚举的可变方法可以把隐式的 `self` 形参设置为同一枚举的另一个成员：

```swift
enum TriStateSwitch {
    case off, low, high
    mutating func next() {
        switch self {
        case .off:
            self = .low
        case .low:
            self = .high
        case .high:
            self = .off
        }
    }
}
var ovenLight = TriStateSwitch.low
ovenLight.next()
// ovenLight is now equal to .high
ovenLight.next()
// ovenLight is now equal to .off
```

这个示例定义了一个用于三态开关的枚举。每次调用它的 `next()` 方法时，开关都会在三种不同的功率状态（`off`、`low` 和 `high`）之间循环切换。

## 类型方法 {#Type-Methods}

如上所述，实例方法是在特定类型的实例上调用的方法。你也可以定义在类型本身上调用的方法，这类方法称为*类型方法*。要标明某个方法是类型方法，请在方法的 `func` 关键字之前写 `static` 关键字。类也可以用 `class` 关键字代替，以允许子类重写父类对该方法的实现。

> 注意：在 Objective-C 中，只能为 Objective-C 类定义类型级别的方法。在 Swift 中，可以为所有类、结构体和枚举定义类型级别的方法。每个类型方法都显式地限定在它所支持的类型范围内。

类型方法和实例方法一样使用点语法调用。不过，类型方法是在类型上调用，而不是在该类型的实例上调用。下面演示如何在名为 `SomeClass` 的类上调用类型方法：

```swift
class SomeClass {
    class func someTypeMethod() {
        // type method implementation goes here
    }
}
SomeClass.someTypeMethod()
```

在类型方法的主体中，隐式的 `self` 属性指的是类型本身，而不是该类型的实例。这意味着你可以用 `self` 来区分类型属性和类型方法的形参，就像用 `self` 区分实例属性和实例方法的形参一样。

更一般地说，你在类型方法主体中使用的任何不带限定的方法名和属性名，指的都是其他的类型级别方法和属性。类型方法可以用另一个类型方法的名字来调用它，无需在名字前加上类型名。同样，结构体和枚举上的类型方法也可以通过类型属性的名字来访问类型属性，无需加上类型名前缀。

下面的示例定义了一个名为 `LevelTracker` 的结构体，用来跟踪玩家在游戏各个关卡（或阶段）中的进度。这是一款单人游戏，但可以在单台设备上存储多个玩家的信息。

游戏第一次运行时，所有关卡（第一关除外）都是锁定的。每当有玩家完成某一关，该关卡就会对这台设备上的所有玩家解锁。`LevelTracker` 结构体使用类型属性和类型方法来记录游戏中的哪些关卡已经解锁，同时还跟踪单个玩家当前所处的关卡。

```swift
struct LevelTracker {
    static var highestUnlockedLevel = 1
    var currentLevel = 1

    static func unlock(_ level: Int) {
        if level > highestUnlockedLevel { highestUnlockedLevel = level }
    }

    static func isUnlocked(_ level: Int) -> Bool {
        return level <= highestUnlockedLevel
    }

    @discardableResult
    mutating func advance(to level: Int) -> Bool {
        if LevelTracker.isUnlocked(level) {
            currentLevel = level
            return true
        } else {
            return false
        }
    }
}
```

`LevelTracker` 结构体记录任何玩家已解锁的最高关卡。这个值存储在名为 `highestUnlockedLevel` 的类型属性中。

`LevelTracker` 还定义了两个类型函数来操作 `highestUnlockedLevel` 属性。第一个是名为 `unlock(_:)` 的类型函数，它会在有新关卡解锁时更新 `highestUnlockedLevel` 的值。第二个是名为 `isUnlocked(_:)` 的便利类型函数，如果某个关卡编号已经解锁，它就返回 `true`。（注意，这些类型方法可以直接访问 `highestUnlockedLevel` 类型属性，你不需要把它写成 `LevelTracker.highestUnlockedLevel`。）

除了类型属性和类型方法，`LevelTracker` 还跟踪单个玩家在游戏中的进度。它使用名为 `currentLevel` 的实例属性来记录玩家当前正在玩的关卡。

为了便于管理 `currentLevel` 属性，`LevelTracker` 定义了一个名为 `advance(to:)` 的实例方法。在更新 `currentLevel` 之前，该方法会检查请求的新关卡是否已经解锁。`advance(to:)` 方法返回一个布尔值，表示它是否真的成功设置了 `currentLevel`。调用 `advance(to:)` 方法的代码忽略返回值并不一定是错误，因此这个函数被标记了 `@discardableResult` 特性。关于这个特性的更多信息，参见 [特性](attributes.md)。

`LevelTracker` 结构体与下面所示的 `Player` 类配合使用，来跟踪和更新单个玩家的进度：

```swift
class Player {
    var tracker = LevelTracker()
    let playerName: String
    func complete(level: Int) {
        LevelTracker.unlock(level + 1)
        tracker.advance(to: level + 1)
    }
    init(name: String) {
        playerName = name
    }
}
```

`Player` 类创建一个新的 `LevelTracker` 实例来跟踪该玩家的进度。它还提供一个名为 `complete(level:)` 的方法，每当玩家完成某一关时就会调用它。这个方法为所有玩家解锁下一关，并更新该玩家的进度，使其进入下一关。（这里忽略了 `advance(to:)` 的布尔返回值，因为上一行对 `LevelTracker.unlock(_:)` 的调用已经确定该关卡被解锁了。）

你可以为新玩家创建一个 `Player` 类的实例，看看该玩家完成第一关时会发生什么：

```swift
var player = Player(name: "Argyrios")
player.complete(level: 1)
print("highest unlocked level is now \(LevelTracker.highestUnlockedLevel)")
// Prints "highest unlocked level is now 2".
```

如果你创建第二个玩家，并尝试把他移到游戏中还没有任何玩家解锁的关卡，那么设置该玩家当前关卡的尝试就会失败：

```swift
player = Player(name: "Beto")
if player.tracker.advance(to: 6) {
    print("player is now on level 6")
} else {
    print("level 6 hasn't yet been unlocked")
}
// Prints "level 6 hasn't yet been unlocked".
```
