# 协议 {#Protocols}

定义遵循类型必须实现的要求。

*协议*（protocol）定义了一个蓝图，其中包含适合某项特定任务或某部分功能的方法、属性和其他要求。此后，类、结构体或枚举可以*采纳*（adopt）该协议，为这些要求提供实际的实现。任何满足协议要求的类型，都被称为*遵循*（conform）该协议。

除了指定遵循类型必须实现的要求之外，你还可以扩展协议，实现其中的某些要求，或者实现遵循类型可以利用的额外功能。

## 协议语法 {#Protocol-Syntax}

你定义协议的方式与定义类、结构体和枚举非常相似：

```swift
protocol SomeProtocol {
    // protocol definition goes here
}
```

自定义类型在其定义中声明采纳某个特定协议，方式是把协议名写在类型名之后，用冒号分隔。可以列出多个协议，用逗号分隔：

```swift
struct SomeStructure: FirstProtocol, AnotherProtocol {
    // structure definition goes here
}
```

如果类有父类，把父类名列在它采纳的任何协议之前，后面跟一个逗号：

```swift
class SomeClass: SomeSuperclass, FirstProtocol, AnotherProtocol {
    // class definition goes here
}
```

> 注意：因为协议是类型，所以协议名要以大写字母开头（例如 `FullyNamed` 和 `RandomNumberGenerator`），与 Swift 中其他类型的名称（例如 `Int`、`String` 和 `Double`）保持一致。

## 属性要求 {#Property-Requirements}

协议可以要求任何遵循类型提供具有特定名称和类型的实例属性或类型属性。协议并不规定该属性应该是存储属性还是计算属性——它只规定所需的属性名称和类型。协议还规定每个属性必须是可读的，还是既可读*又*可写。

如果协议要求某个属性既可读又可写，那么该属性要求不能由常量存储属性或只读计算属性来满足。如果协议只要求某个属性可读，那么任何类型的属性都能满足该要求；如果让该属性同时可写对你的代码有用，那也是有效的。

属性要求总是声明为变量属性，并在前面加上 `var` 关键字。可读且可写的属性通过在类型声明后面写 `{ get set }` 来表示，可读的属性则通过写 `{ get }` 来表示。

```swift
protocol SomeProtocol {
    var mustBeSettable: Int { get set }
    var doesNotNeedToBeSettable: Int { get }
}
```

在协议中定义类型属性要求时，一定要在它们前面加上 `static` 关键字。即使在类实现这些要求时可以用 `class` 或 `static` 关键字作为前缀，这条规则仍然适用：

```swift
protocol AnotherProtocol {
    static var someTypeProperty: Int { get set }
}
```

下面是一个只包含单个实例属性要求的协议示例：

```swift
protocol FullyNamed {
    var fullName: String { get }
}
```

`FullyNamed` 协议要求遵循类型提供一个完全限定的名称。协议对该遵循类型的性质不作其他规定——它只规定该类型必须能够为自身提供一个完整名称。协议声明任何 `FullyNamed` 类型都必须有一个名为 `fullName`、类型为 `String` 的可读实例属性。

下面是一个简单的结构体示例，它采纳并遵循 `FullyNamed` 协议：

```swift
struct Person: FullyNamed {
    var fullName: String
}
let john = Person(fullName: "John Appleseed")
// john.fullName is "John Appleseed"
```

这个示例定义了一个名为 `Person` 的结构体，表示一个具体的有名字的人。它在定义的第一行声明采纳 `FullyNamed` 协议。

`Person` 的每个实例都只有一个名为 `fullName`、类型为 `String` 的存储属性。这符合 `FullyNamed` 协议的唯一要求，意味着 `Person` 正确地遵循了该协议。（如果协议要求没有得到满足，Swift 会在编译时报错。）

下面是一个更复杂的类，它也采纳并遵循 `FullyNamed` 协议：

```swift
class Starship: FullyNamed {
    var prefix: String?
    var name: String
    init(name: String, prefix: String? = nil) {
        self.name = name
        self.prefix = prefix
    }
    var fullName: String {
        return (prefix != nil ? prefix! + " " : "") + name
    }
}
var ncc1701 = Starship(name: "Enterprise", prefix: "USS")
// ncc1701.fullName is "USS Enterprise"
```

这个类把 `fullName` 属性要求实现为飞船的一个只读计算属性。每个 `Starship` 类实例都存储一个必需的 `name` 和一个可选的 `prefix`。`fullName` 属性会在 `prefix` 存在时使用它的值，并把它前置到 `name` 开头，从而构造出飞船的完整名称。

## 方法要求 {#Method-Requirements}

协议可以要求遵循类型实现特定的实例方法和类型方法。这些方法的写法与普通实例方法和类型方法完全相同，作为协议定义的一部分，但不带花括号或方法体。可变参数是允许的，遵循与普通方法相同的规则。但是，在协议定义中不能为方法形参指定默认值。

与类型属性要求一样，在协议中定义类型方法要求时，一定要在它们前面加上 `static` 关键字。即使类在实现这些类型方法要求时可以用 `class` 或 `static` 关键字作为前缀，这一点仍然成立：

```swift
protocol SomeProtocol {
    static func someTypeMethod()
}
```

下面的示例定义了一个只包含单个实例方法要求的协议：

```swift
protocol RandomNumberGenerator {
    func random() -> Double
}
```

这个 `RandomNumberGenerator` 协议要求任何遵循类型都有一个名为 `random` 的实例方法，每次调用时返回一个 `Double` 值。虽然协议中没有规定，但可以假定这个值是 `0.0` 到 `1.0`（不包括 `1.0`）之间的一个数。

`RandomNumberGenerator` 协议不对每个随机数如何生成作任何假定——它只要求生成器提供一种生成新随机数的标准方式。

下面是一个类的实现，它采纳并遵循 `RandomNumberGenerator` 协议。这个类实现了一种称为*线性同余生成器*（linear congruential generator）的伪随机数生成算法：

```swift
class LinearCongruentialGenerator: RandomNumberGenerator {
    var lastRandom = 42.0
    let m = 139968.0
    let a = 3877.0
    let c = 29573.0
    func random() -> Double {
        lastRandom = ((lastRandom * a + c)
            .truncatingRemainder(dividingBy:m))
        return lastRandom / m
    }
}
let generator = LinearCongruentialGenerator()
print("Here's a random number: \(generator.random())")
// Prints "Here's a random number: 0.3746499199817101".
print("And another one: \(generator.random())")
// Prints "And another one: 0.729023776863283".
```

## 可变方法要求 {#Mutating-Method-Requirements}

有时方法需要修改（或*改变*）它所属的实例。对于值类型（即结构体和枚举）上的实例方法，要把 `mutating` 关键字放在方法的 `func` 关键字之前，以表明该方法可以修改它所属的实例以及该实例的任何属性。这一过程在 [在实例方法中修改值类型](methods.md#Modifying-Value-Types-from-Within-Instance-Methods) 中有描述。

如果你定义的协议实例方法要求意在改变采纳该协议的任何类型的实例，就要在协议定义中给该方法标上 `mutating` 关键字。这样结构体和枚举才能采纳该协议并满足该方法要求。

> 注意：如果把协议实例方法要求标记为 `mutating`，那么为类编写该方法的实现时不需要写 `mutating` 关键字。`mutating` 关键字只由结构体和枚举使用。

下面的示例定义了一个名为 `Togglable` 的协议，它定义了一个名为 `toggle` 的实例方法要求。顾名思义，`toggle()` 方法旨在切换或反转任何遵循类型的状态，通常是通过修改该类型的某个属性来实现。

在 `Togglable` 协议定义中，`toggle()` 方法标有 `mutating` 关键字，以表明该方法在被调用时应改变遵循类型的实例的状态：

```swift
protocol Togglable {
    mutating func toggle()
}
```

如果你为结构体或枚举实现 `Togglable` 协议，那么该结构体或枚举可以通过提供一个同样标记为 `mutating` 的 `toggle()` 方法实现来遵循该协议。

下面的示例定义了一个名为 `OnOffSwitch` 的枚举。这个枚举在两个状态之间切换，这两个状态由枚举成员 `on` 和 `off` 表示。该枚举的 `toggle` 实现标记为 `mutating`，以符合 `Togglable` 协议的要求：

```swift
enum OnOffSwitch: Togglable {
    case off, on
    mutating func toggle() {
        switch self {
        case .off:
            self = .on
        case .on:
            self = .off
        }
    }
}
var lightSwitch = OnOffSwitch.off
lightSwitch.toggle()
// lightSwitch is now equal to .on
```

## 构造器要求 {#Initializer-Requirements}

协议可以要求遵循类型实现特定的构造器。这些构造器的写法与普通构造器完全相同，作为协议定义的一部分，但不带花括号或构造器体：

```swift
protocol SomeProtocol {
    init(someParameter: Int)
}
```

### 协议构造器要求的类实现 {#Class-Implementations-of-Protocol-Initializer-Requirements}

你可以在遵循协议的类上，把协议构造器要求实现为指定构造器或便利构造器。两种情况下，都必须用 `required` 修饰符标记该构造器实现：

```swift
class SomeClass: SomeProtocol {
    required init(someParameter: Int) {
        // initializer implementation goes here
    }
}
```

使用 `required` 修饰符可以确保在遵循类的所有子类上提供该构造器要求的显式或继承实现，从而让这些子类也遵循该协议。

关于必要构造器的更多信息，参见 [必要构造器](initialization.md#Required-Initializers)。

> 注意：在标记了 `final` 修饰符的类上，不需要用 `required` 修饰符标记协议构造器实现，因为 final 类不能被继承。关于 `final` 修饰符的更多信息，参见 [阻止重写](inheritance.md#Preventing-Overrides)。

如果子类重写了父类的指定构造器，同时还实现了协议中与之匹配的构造器要求，就要用 `required` 和 `override` 两个修饰符标记该构造器实现：

```swift
protocol SomeProtocol {
    init()
}

class SomeSuperClass {
    init() {
        // initializer implementation goes here
    }
}

class SomeSubClass: SomeSuperClass, SomeProtocol {
    // "required" from SomeProtocol conformance; "override" from SomeSuperClass
    required override init() {
        // initializer implementation goes here
    }
}
```

### 可失败构造器要求 {#Failable-Initializer-Requirements}

协议可以为遵循类型定义可失败构造器要求，如 [可失败构造器](initialization.md#Failable-Initializers) 中所定义。

可失败构造器要求可以由遵循类型上的可失败构造器或非可失败构造器来满足。非可失败构造器要求可以由非可失败构造器或隐式解包的可失败构造器来满足。

## 仅有语义要求的协议 {#Protocols-that-Have-Only-Semantic-Requirements}

上面所有示例协议都要求一些方法或属性，但协议声明不必包含任何要求。你还可以用协议来描述*语义*要求——即关于这些类型的值如何表现，以及它们支持哪些操作的要求。

Swift 标准库定义了若干个没有任何必需方法或属性的协议：

- [`Sendable`][] 用于可以在并发域之间共享的值，详见 [可发送类型](concurrency.md#Sendable-Types)。
- [`Copyable`][] 用于当你把它们传入函数时 Swift 可以复制的值，详见 [借用参数与消耗参数](declarations.md#Borrowing-and-Consuming-Parameters)。
- [`BitwiseCopyable`][] 用于可以逐位复制的值。

[`BitwiseCopyable`]: https://developer.apple.com/documentation/swift/bitwisecopyable [`Copyable`]: https://developer.apple.com/documentation/swift/copyable [`Sendable`]: https://developer.apple.com/documentation/swift/sendable

关于这些协议要求的更多信息，参见其文档中的概述。

采纳这些协议所用的语法与采纳其他协议相同。唯一的区别是，不需要包含实现协议要求的方法或属性声明。例如：

```swift
struct MyStruct: Copyable {
    var counter = 12
}

extension MyStruct: BitwiseCopyable { }
```

上面的代码定义了一个新结构体。因为 `Copyable` 只有语义要求，所以该结构体声明中没有任何采纳该协议的代码。同样，因为 `BitwiseCopyable` 只有语义要求，采纳该协议的扩展主体为空。

通常你不需要写出对这些协议的一致性——Swift 会为你隐式添加一致性，详见 [协议的隐式一致性](protocols.md#Implicit-Conformance-to-a-Protocol)。

## 协议作为类型 {#Protocols-as-Types}

协议本身实际上不实现任何功能。尽管如此，你仍然可以在代码中把协议用作类型。

把协议用作类型最常见的方式，是把协议用作泛型约束。带泛型约束的代码可以与任何遵循该协议的类型协作，而具体类型由使用该 API 的代码选择。例如，当你调用一个接收实参、且该实参的类型是泛型的函数时，由调用方选择类型。

带不透明类型的代码与某个遵循该协议的类型协作。底层类型在编译时已知，由 API 实现选择该类型，但该类型的标识对 API 的客户端是隐藏的。使用不透明类型可以防止 API 的实现细节通过抽象层泄漏出去——例如，隐藏函数的特定返回类型，只保证该值遵循某个给定协议。

带装箱协议类型的代码与任何在运行时选定的、遵循该协议的类型协作。为了支持这种运行时的灵活性，Swift 在必要时会增加一层间接——称为*装箱*（box），这会带来性能开销。由于这种灵活性，Swift 在编译时不知道底层类型，这意味着你只能访问协议所要求的成员。要访问底层类型上的任何其他 API，需要在运行时进行转换。

关于把协议用作泛型约束的信息，参见 [泛型](generics.md)。关于不透明类型和装箱协议类型的信息，参见 [不透明类型与装箱协议类型](opaque-types.md)。

## 委托 {#Delegation}

*委托*（delegation）是一种设计模式，让类或结构体把部分职责交给（或*委托*给）另一个类型的实例。实现这种设计模式的方式是定义一个协议来封装被委托的职责，从而保证遵循该协议的类型（称为委托对象）一定提供被委托的功能。委托可以用来响应某个特定动作，或者从外部来源获取数据，而不需要知道该来源的底层类型。

下面的示例定义了一个掷骰子游戏，以及一个用于跟踪游戏进度的委托对象的嵌套协议：

```swift
class DiceGame {
    let sides: Int
    let generator = LinearCongruentialGenerator()
    weak var delegate: Delegate?

    init(sides: Int) {
        self.sides = sides
    }

    func roll() -> Int {
        return Int(generator.random() * Double(sides)) + 1
    }

    func play(rounds: Int) {
        delegate?.gameDidStart(self)
        for round in 1...rounds {
            let player1 = roll()
            let player2 = roll()
            if player1 == player2 {
                delegate?.game(self, didEndRound: round, winner: nil)
            } else if player1 > player2 {
                delegate?.game(self, didEndRound: round, winner: 1)
            } else {
                delegate?.game(self, didEndRound: round, winner: 2)
            }
        }
        delegate?.gameDidEnd(self)
    }

    protocol Delegate: AnyObject {
        func gameDidStart(_ game: DiceGame)
        func game(_ game: DiceGame, didEndRound round: Int, winner: Int?)
        func gameDidEnd(_ game: DiceGame)
    }
}
```

`DiceGame` 类实现了一个游戏：每位玩家轮流掷骰子，掷出最大数字的玩家赢得该轮。它使用本章前面示例中的线性同余生成器来为掷骰子生成随机数。

可以采纳 `DiceGame.Delegate` 协议来跟踪掷骰子游戏的进度。由于 `DiceGame.Delegate` 协议总是在掷骰子游戏的上下文中使用，它嵌套在 `DiceGame` 类内部。协议可以嵌套在结构体和类这样的类型声明内部，只要外层声明不是泛型的。关于嵌套类型的信息，参见 [嵌套类型](nested-types.md)。

为了防止强引用循环，委托对象被声明为弱引用。关于弱引用的信息，参见 [类实例之间的强引用循环](automatic-reference-counting.md#Strong-Reference-Cycles-Between-Class-Instances)。把协议标记为仅类协议，可以让 `DiceGame` 类声明其委托对象必须使用弱引用。仅类协议通过继承 `AnyObject` 来标记，详见 [仅类协议](protocols.md#Class-Only-Protocols)。

`DiceGame.Delegate` 提供了三个用于跟踪游戏进度的方法。这三个方法被整合到上面 `play(rounds:)` 方法的游戏逻辑中。`DiceGame` 类会在新游戏开始、新一轮开始或游戏结束时调用其委托方法。

因为 `delegate` 属性是一个*可选*的 `DiceGame.Delegate`，所以 `play(rounds:)` 方法每次调用委托对象上的方法时都会使用可选链，详见 [可选链](optional-chaining.md)。如果 `delegate` 属性为 nil，这些委托调用会被忽略。如果 `delegate` 属性不为 nil，就会调用委托方法，并把 `DiceGame` 实例作为参数传入。

下一个示例展示了一个名为 `DiceGameTracker` 的类，它采纳 `DiceGame.Delegate` 协议：

```swift
class DiceGameTracker: DiceGame.Delegate {
    var playerScore1 = 0
    var playerScore2 = 0
    func gameDidStart(_ game: DiceGame) {
        print("Started a new game")
        playerScore1 = 0
        playerScore2 = 0
    }
    func game(_ game: DiceGame, didEndRound round: Int, winner: Int?) {
        switch winner {
            case 1:
                playerScore1 += 1
                print("Player 1 won round \(round)")
            case 2: playerScore2 += 1
                print("Player 2 won round \(round)")
            default:
                print("The round was a draw")
        }
    }
    func gameDidEnd(_ game: DiceGame) {
        if playerScore1 == playerScore2 {
            print("The game ended in a draw.")
        } else if playerScore1 > playerScore2 {
            print("Player 1 won!")
        } else {
            print("Player 2 won!")
        }
    }
}
```

`DiceGameTracker` 类实现了 `DiceGame.Delegate` 协议要求的全部三个方法。它用这些方法在新游戏开始时把两位玩家的分数清零，在每轮结束时更新他们的分数，并在游戏结束时宣布获胜者。

下面展示了 `DiceGame` 和 `DiceGameTracker` 在实际运行中的样子：

```swift
let tracker = DiceGameTracker()
let game = DiceGame(sides: 6)
game.delegate = tracker
game.play(rounds: 3)
// Started a new game
// Player 2 won round 1
// Player 2 won round 2
// Player 1 won round 3
// Player 2 won!
```

## 用扩展添加协议一致性 {#Adding-Protocol-Conformance-with-an-Extension}

你可以扩展现有类型，使其采纳并遵循一个新协议，即使你无法访问该现有类型的源代码。扩展可以为现有类型添加新的属性、方法和下标，因此能够添加协议可能要求的任何要求。关于扩展的更多信息，参见 [扩展](extensions.md)。

> 注意：当在扩展中为某个类型添加协议一致性时，该类型的现有实例会自动采纳并遵循该协议。

例如，下面这个名为 `TextRepresentable` 的协议可以由任何能以文本形式表示自身的类型来实现。这种表示可以是关于自身的描述，也可以是自身当前状态的文本版本：

```swift
protocol TextRepresentable {
    var textualDescription: String { get }
}
```

可以扩展上面的 `Dice` 类，使其采纳并遵循 `TextRepresentable`：

```swift
extension Dice: TextRepresentable {
    var textualDescription: String {
        return "A \(sides)-sided dice"
    }
}
```

这个扩展采纳新协议的方式，与 `Dice` 在其原始实现中采纳该协议时完全相同。协议名写在类型名之后，用冒号分隔，并在扩展的花括号内提供协议所有要求的实现。

现在，任何 `Dice` 实例都可以当作 `TextRepresentable` 来使用：

```swift
let d12 = Dice(sides: 12, generator: LinearCongruentialGenerator())
print(d12.textualDescription)
// Prints "A 12-sided dice".
```

同样，可以扩展 `SnakesAndLadders` 游戏类，使其采纳并遵循 `TextRepresentable` 协议：

```swift
extension SnakesAndLadders: TextRepresentable {
    var textualDescription: String {
        return "A game of Snakes and Ladders with \(finalSquare) squares"
    }
}
print(game.textualDescription)
// Prints "A game of Snakes and Ladders with 25 squares".
```

### 有条件地遵循协议 {#Conditionally-Conforming-to-a-Protocol}

泛型类型可能只有在某些条件下才能满足协议的要求，例如当该类型的泛型形参遵循该协议时。你可以通过在扩展类型时列出约束，让泛型类型有条件地遵循某个协议。这些约束写在你要采纳的协议名之后，通过编写泛型 `where` 子句来实现。关于泛型 `where` 子句的更多信息，参见 [泛型 where 子句](generics.md#Generic-Where-Clauses)。

下面的扩展使 `Array` 实例在其存储的元素类型遵循 `TextRepresentable` 时遵循 `TextRepresentable` 协议。

```swift
extension Array: TextRepresentable where Element: TextRepresentable {
    var textualDescription: String {
        let itemsAsText = self.map { $0.textualDescription }
        return "[" + itemsAsText.joined(separator: ", ") + "]"
    }
}
let myDice = [d6, d12]
print(myDice.textualDescription)
// Prints "[A 6-sided dice, A 12-sided dice]".
```

### 用扩展声明协议采纳 {#Declaring-Protocol-Adoption-with-an-Extension}

如果一个类型已经满足某个协议的所有要求，但还没有声明采纳该协议，你可以用一个空扩展让它采纳该协议：

```swift
struct Hamster {
    var name: String
    var textualDescription: String {
        return "A hamster named \(name)"
    }
}
extension Hamster: TextRepresentable {}
```

现在，凡是需要 `TextRepresentable` 类型的地方，都可以使用 `Hamster` 的实例：

```swift
let simonTheHamster = Hamster(name: "Simon")
let somethingTextRepresentable: TextRepresentable = simonTheHamster
print(somethingTextRepresentable.textualDescription)
// Prints "A hamster named Simon".
```

> 注意：类型不会仅仅因为满足了协议的要求就自动采纳该协议。它们必须始终显式声明自己采纳了该协议。

## 使用合成实现采纳协议 {#Adopting-a-Protocol-Using-a-Synthesized-Implementation}

Swift 可以在许多简单情况下自动为 `Equatable`、`Hashable` 和 `Comparable` 提供协议一致性。使用这种合成实现意味着你不必编写重复的样板代码来自己实现协议要求。

Swift 为以下种类的自定义类型提供 `Equatable` 的合成实现：

- 只含有遵循 `Equatable` 协议的存储属性的结构体
- 只含有遵循 `Equatable` 协议的关联类型的枚举
- 没有关联类型的枚举

要获得 `==` 的合成实现，请在包含原始声明的文件中声明遵循 `Equatable`，而不要自己实现 `==` 运算符。`Equatable` 协议提供了 `!=` 的默认实现。

下面的示例定义了一个 `Vector3D` 结构体，用于表示三维位置向量 `(x, y, z)`，它类似于 `Vector2D` 结构体。因为 `x`、`y` 和 `z` 属性都是 `Equatable` 类型，所以 `Vector3D` 会获得等价运算符的合成实现。

```swift
struct Vector3D: Equatable {
    var x = 0.0, y = 0.0, z = 0.0
}

let twoThreeFour = Vector3D(x: 2.0, y: 3.0, z: 4.0)
let anotherTwoThreeFour = Vector3D(x: 2.0, y: 3.0, z: 4.0)
if twoThreeFour == anotherTwoThreeFour {
    print("These two vectors are also equivalent.")
}
// Prints "These two vectors are also equivalent."
```

Swift 为以下种类的自定义类型提供 `Hashable` 的合成实现：

- 只含有遵循 `Hashable` 协议的存储属性的结构体
- 只含有遵循 `Hashable` 协议的关联类型的枚举
- 没有关联类型的枚举

要获得 `hash(into:)` 的合成实现，请在包含原始声明的文件中声明遵循 `Hashable`，而不要自己实现 `hash(into:)` 方法。

Swift 为没有原始值的枚举提供 `Comparable` 的合成实现。如果枚举有关联类型，这些关联类型都必须遵循 `Comparable` 协议。要获得 `<` 的合成实现，请在包含原始枚举声明的文件中声明遵循 `Comparable`，而不要自己实现 `<` 运算符。`Comparable` 协议对 `<=`、`>` 和 `>=` 的默认实现提供了其余的比较运算符。

下面的示例定义了一个 `SkillLevel` 枚举，其中包含初学者、中级和专家这几个成员。专家还会根据其拥有的星数进一步排名。

```swift
enum SkillLevel: Comparable {
    case beginner
    case intermediate
    case expert(stars: Int)
}
var levels = [SkillLevel.intermediate, SkillLevel.beginner,
              SkillLevel.expert(stars: 5), SkillLevel.expert(stars: 3)]
for level in levels.sorted() {
    print(level)
}
// Prints "beginner".
// Prints "intermediate".
// Prints "expert(stars: 3)".
// Prints "expert(stars: 5)".
```

## 协议的隐式一致性 {#Implicit-Conformance-to-a-Protocol}

有些协议非常常见，以至于你几乎每次声明新类型时都会写上它们。对于以下协议，当你定义的类型实现了协议要求时，Swift 会自动推断出一致性，因此你不必自己写出它们：

- [`Copyable`][]
- [`Sendable`][]
- [`BitwiseCopyable`][]

你仍然可以显式写出该一致性，但这不会改变代码的行为。要抑制某个隐式一致性，请在一致性列表中把波浪号（`~`）写在协议名之前：

```swift
struct FileDescriptor: ~Sendable {
    let rawValue: Int
}
```

上面的代码展示了 POSIX 文件描述符包装器的一部分。`FileDescriptor` 结构体满足 `Sendable` 协议的所有要求，这通常会让它成为可发送的。然而，写 `~Sendable` 会抑制这一隐式一致性。尽管文件描述符使用整数来标识打开的文件并与之交互，而且整数值是可发送的，但让它不可发送有助于避免某些种类的 bug。

抑制隐式一致性的另一种方式，是使用一个标记为不可用的扩展：

```swift
@available(*, unavailable)
extension FileDescriptor: Sendable { }
```

当你在代码中的某一处写 `~Sendable`（如上一个示例所示）时，程序中其他地方的代码仍然可以扩展 `FileDescriptor` 类型来添加 `Sendable` 一致性。相比之下，本示例中不可用的扩展会抑制对 `Sendable` 的隐式一致性，并且还会阻止代码中其他地方的任何扩展为该类型添加 `Sendable` 一致性。

> 注意：除了上面讨论的协议之外，分布式 actor 还会隐式遵循 [`Codable`][] 协议。

[`Codable`]: https://developer.apple.com/documentation/swift/codable

## 协议类型的集合 {#Collections-of-Protocol-Types}

如 [协议作为类型](protocols.md#Protocols-as-Types) 中所述，协议可以用作存储在数组或字典等集合中的类型。这个示例创建了一个由 `TextRepresentable` 事物组成的数组：

```swift
let things: [TextRepresentable] = [game, d12, simonTheHamster]
```

现在可以遍历数组中的项，并打印每一项的文本描述：

```swift
for thing in things {
    print(thing.textualDescription)
}
// A game of Snakes and Ladders with 25 squares
// A 12-sided dice
// A hamster named Simon
```

注意，`thing` 常量的类型是 `TextRepresentable`。它不是 `Dice`、`DiceGame` 或 `Hamster` 类型，即使背后实际的实例是这些类型之一。尽管如此，因为它的类型是 `TextRepresentable`，而任何 `TextRepresentable` 都已知有一个 `textualDescription` 属性，所以在每次循环中访问 `thing.textualDescription` 都是安全的。

## 协议继承 {#Protocol-Inheritance}

协议可以*继承*一个或多个其他协议，并可以在所继承要求的基础上添加更多要求。协议继承的语法与类继承的语法相似，但可以选择列出多个被继承的协议，用逗号分隔：

```swift
protocol InheritingProtocol: SomeProtocol, AnotherProtocol {
    // protocol definition goes here
}
```

下面是一个继承上面 `TextRepresentable` 协议的协议示例：

```swift
protocol PrettyTextRepresentable: TextRepresentable {
    var prettyTextualDescription: String { get }
}
```

这个示例定义了一个新协议 `PrettyTextRepresentable`，它继承自 `TextRepresentable`。任何采纳 `PrettyTextRepresentable` 的类型都必须满足 `TextRepresentable` 强制执行的所有要求，*外加* `PrettyTextRepresentable` 强制执行的额外要求。在这个示例中，`PrettyTextRepresentable` 只添加了一项要求：提供一个名为 `prettyTextualDescription`、返回 `String` 的可读属性。

可以扩展 `SnakesAndLadders` 类，使其采纳并遵循 `PrettyTextRepresentable`：

```swift
extension SnakesAndLadders: PrettyTextRepresentable {
    var prettyTextualDescription: String {
        var output = textualDescription + ":\n"
        for index in 1...finalSquare {
            switch board[index] {
            case let ladder where ladder > 0:
                output += "▲ "
            case let snake where snake < 0:
                output += "▼ "
            default:
                output += "○ "
            }
        }
        return output
    }
}
```

这个扩展声明采纳 `PrettyTextRepresentable` 协议，并为 `SnakesAndLadders` 类型提供 `prettyTextualDescription` 属性的实现。任何 `PrettyTextRepresentable` 类型也必须是 `TextRepresentable`，因此 `prettyTextualDescription` 的实现首先访问来自 `TextRepresentable` 协议的 `textualDescription` 属性，以此作为输出字符串的开头。它追加一个冒号和一个换行符，并将其作为精美文本表示的开头。然后，它遍历棋盘方格数组，并追加一个几何形状来表示每个方格的内容：

- 如果方格的值大于 `0`，它就是梯子的底部，用 `▲` 表示。
- 如果方格的值小于 `0`，它就是蛇头，用 `▼` 表示。
- 否则，方格的值就是 `0`，它是一个“自由”方格，用 `○` 表示。

现在可以用 `prettyTextualDescription` 属性打印任何 `SnakesAndLadders` 实例的精美文本描述：

```swift
print(game.prettyTextualDescription)
// A game of Snakes and Ladders with 25 squares:
// ○ ○ ▲ ○ ○ ▲ ○ ○ ▲ ▲ ○ ○ ○ ▼ ○ ○ ○ ○ ▼ ○ ○ ▼ ○ ▼ ○
```

## 仅类协议 {#Class-Only-Protocols}

你可以通过把 `AnyObject` 协议添加到协议的继承列表中，把协议的采纳限制为类类型（而不是结构体或枚举）。

```swift
protocol SomeClassOnlyProtocol: AnyObject, SomeInheritedProtocol {
    // class-only protocol definition goes here
}
```

在上面的示例中，`SomeClassOnlyProtocol` 只能被类类型采纳。编写试图采纳 `SomeClassOnlyProtocol` 的结构体或枚举定义会导致编译时错误。

> 注意：当该协议的要求所定义的行为假定或要求遵循类型具有引用语义而非值语义时，请使用仅类协议。关于引用语义和值语义的更多信息，参见 [结构体和枚举是值类型](classes-and-structures.md#Structures-and-Enumerations-Are-Value-Types) 和 [类是引用类型](classes-and-structures.md#Classes-Are-Reference-Types)。

你还可以在协议的继承列表中指定某个特定类，以要求所有遵循类型都是该类的子类。

```swift
class Renderer {
    func render(_ text: String) {
        print(text)
    }
}

protocol Widget: Renderer {
    var title: String { get }
}

extension Widget {
    func display() {
        render(title)
    }
}

class ButtonWidget: Renderer, Widget {
    var title: String
    init(title: String) {
        self.title = title
    }
}

let button = ButtonWidget(title: "OK")
button.display()
// Prints "OK".
```

在上面的示例中，`Widget` 只能被 `Renderer` 的子类采纳。因为该协议保证每个遵循类型都是 `Renderer` 的子类，所以 `Widget` 协议扩展可以直接调用 `render(_:)`。父类要求比使用 `AnyObject` 限制性更强：它把一致性限制在特定的类层次结构中，而不只是任何类类型。当协议实现依赖某个特定基类的具体行为时，请使用这种方式。

## 协议组合 {#Protocol-Composition}

要求一个类型同时遵循多个协议可能很有用。你可以用*协议组合*（protocol composition）把多个协议合并成单个要求。协议组合的行为就像你定义了一个临时的局部协议，该协议具有组合中所有协议的合并要求。协议组合不定义任何新的协议类型。

协议组合的形式为 `SomeProtocol & AnotherProtocol`。你可以按需列出任意多个协议，用与号（`&`）分隔。除了协议列表之外，协议组合还可以包含一个类类型，你可以用它来指定所需的父类。

下面是一个示例，它把名为 `Named` 和 `Aged` 的两个协议合并为函数参数上的单个协议组合要求：

```swift
protocol Named {
    var name: String { get }
}
protocol Aged {
    var age: Int { get }
}
struct Person: Named, Aged {
    var name: String
    var age: Int
}
func wishHappyBirthday(to celebrator: Named & Aged) {
    print("Happy birthday, \(celebrator.name), you're \(celebrator.age)!")
}
let birthdayPerson = Person(name: "Malcolm", age: 21)
wishHappyBirthday(to: birthdayPerson)
// Prints "Happy birthday, Malcolm, you're 21!"
```

在这个示例中，`Named` 协议只有一项要求：提供一个名为 `name`、类型为 `String` 的可读属性。`Aged` 协议只有一项要求：提供一个名为 `age`、类型为 `Int` 的可读属性。这两个协议都被一个名为 `Person` 的结构体采纳。

这个示例还定义了一个 `wishHappyBirthday(to:)` 函数。`celebrator` 形参的类型是 `Named & Aged`，意思是“任何同时遵循 `Named` 和 `Aged` 两个协议的类型”。传给该函数的具体类型是什么并不重要，只要它同时遵循这两个必需协议即可。

然后，这个示例创建了一个名为 `birthdayPerson` 的新 `Person` 实例，并把这个新实例传给 `wishHappyBirthday(to:)` 函数。因为 `Person` 同时遵循这两个协议，所以这次调用是有效的，`wishHappyBirthday(to:)` 函数可以打印它的生日祝福。

下面是一个示例，它把上一个示例中的 `Named` 协议与一个 `Location` 类组合起来：

```swift
class Location {
    var latitude: Double
    var longitude: Double
    init(latitude: Double, longitude: Double) {
        self.latitude = latitude
        self.longitude = longitude
    }
}
class City: Location, Named {
    var name: String
    init(name: String, latitude: Double, longitude: Double) {
        self.name = name
        super.init(latitude: latitude, longitude: longitude)
    }
}
func beginConcert(in location: Location & Named) {
    print("Hello, \(location.name)!")
}

let seattle = City(name: "Seattle", latitude: 47.6, longitude: -122.3)
beginConcert(in: seattle)
// Prints "Hello, Seattle!"
```

`beginConcert(in:)` 函数接收一个类型为 `Location & Named` 的参数，意思是“任何是 `Location` 子类且遵循 `Named` 协议的类型”。在这个例子中，`City` 同时满足这两项要求。

把 `birthdayPerson` 传给 `beginConcert(in:)` 函数是无效的，因为 `Person` 不是 `Location` 的子类。同样，如果你创建了一个不遵循 `Named` 协议的 `Location` 子类，那么用该类型的实例调用 `beginConcert(in:)` 也是无效的。

## 检查协议一致性 {#Checking-for-Protocol-Conformance}

你可以使用 [类型转换](type-casting.md) 中描述的 `is` 和 `as` 运算符来检查协议一致性，并转换到特定协议。检查协议一致性以及转换到协议的语法，与检查类型以及转换到类型的语法完全相同：

- `is` 运算符在实例遵循某个协议时返回 `true`，不遵循时返回 `false`。
- 向下转型运算符的 `as?` 版本返回协议类型的可选值；如果实例不遵循该协议，这个值就是 `nil`。
- 向下转型运算符的 `as!` 版本强制向下转型为协议类型；如果转型不成功，会触发运行时错误。

这个示例定义了一个名为 `HasArea` 的协议，它只有一项属性要求：提供一个名为 `area`、类型为 `Double` 的可读属性：

```swift
protocol HasArea {
    var area: Double { get }
}
```

下面是两个类 `Circle` 和 `Country`，它们都遵循 `HasArea` 协议：

```swift
class Circle: HasArea {
    let pi = 3.1415927
    var radius: Double
    var area: Double { return pi * radius * radius }
    init(radius: Double) { self.radius = radius }
}
class Country: HasArea {
    var area: Double
    init(area: Double) { self.area = area }
}
```

`Circle` 类把 `area` 属性要求实现为一个基于存储属性 `radius` 的计算属性。`Country` 类直接把 `area` 要求实现为存储属性。这两个类都正确地遵循了 `HasArea` 协议。

下面是一个名为 `Animal` 的类，它不遵循 `HasArea` 协议：

```swift
class Animal {
    var legs: Int
    init(legs: Int) { self.legs = legs }
}
```

`Circle`、`Country` 和 `Animal` 这三个类没有共同的基类。尽管如此，它们都是类，因此这三种类型的实例都可以用来初始化一个存储 `AnyObject` 类型值的数组：

```swift
let objects: [AnyObject] = [
    Circle(radius: 2.0),
    Country(area: 243_610),
    Animal(legs: 4)
]
```

`objects` 数组使用一个数组字面量初始化，其中包含：一个半径为 2 个单位的 `Circle` 实例；一个以英国的表面积（单位为平方公里）初始化的 `Country` 实例；以及一个有四条腿的 `Animal` 实例。

现在可以遍历 `objects` 数组，并检查数组中的每个对象是否遵循 `HasArea` 协议：

```swift
for object in objects {
    if let objectWithArea = object as? HasArea {
        print("Area is \(objectWithArea.area)")
    } else {
        print("Something that doesn't have an area")
    }
}
// Area is 12.5663708
// Area is 243610.0
// Something that doesn't have an area
```

每当数组中的某个对象遵循 `HasArea` 协议时，`as?` 运算符返回的可选值就会通过可选绑定解包到一个名为 `objectWithArea` 的常量中。`objectWithArea` 常量的类型已知为 `HasArea`，因此可以以类型安全的方式访问并打印它的 `area` 属性。

注意，转换过程不会改变底层对象。它们仍然是 `Circle`、`Country` 和 `Animal`。然而，当它们被存入 `objectWithArea` 常量时，只知道它们的类型是 `HasArea`，因此只能访问它们的 `area` 属性。

## 可选协议要求 {#Optional-Protocol-Requirements}

你可以为协议定义*可选要求*（optional requirements）。遵循该协议的类型不必实现这些要求。可选要求在协议定义中以 `optional` 修饰符作为前缀。提供可选要求是为了让你能编写与 Objective-C 互操作的代码。协议和可选要求都必须用 `@objc` 特性标记。注意，`@objc` 协议只能被类采纳，不能被结构体或枚举采纳。

当你使用可选要求中的方法或属性时，它的类型会自动变成可选类型。例如，类型为 `(Int) -> String` 的方法会变成 `((Int) -> String)?`。注意，被包装成可选的是整个函数类型，而不是方法的返回值。

可选协议要求可以用可选链来调用，以应对遵循该协议的类型没有实现该要求的情况。检查某个可选方法是否已实现的方式是：调用该方法时在方法名后写一个问号，例如 `someOptionalMethod?(someArgument)`。关于可选链的信息，参见 [可选链](optional-chaining.md)。

下面的示例定义了一个名为 `Counter` 的整数计数类，它使用外部数据源来提供增量。这个数据源由 `CounterDataSource` 协议定义，该协议有两个可选要求：

```swift
```

`CounterDataSource` 协议定义了一个名为 `increment(forCount:)` 的可选方法要求，以及一个名为 `fixedIncrement` 的可选属性要求。这些要求定义了数据源为 `Counter` 实例提供合适增量的两种不同方式。

> 注意：严格来说，你可以编写一个遵循 `CounterDataSource` 的自定义类，而*两个*协议要求都不实现。毕竟它们都是可选的。虽然技术上允许，但这样的数据源并不好用。

下面定义的 `Counter` 类有一个类型为 `CounterDataSource?` 的可选 `dataSource` 属性：

```swift
class Counter {
    var count = 0
    var dataSource: CounterDataSource?
    func increment() {
        if let amount = dataSource?.increment?(forCount: count) {
            count += amount
        } else if let amount = dataSource?.fixedIncrement {
            count += amount
        }
    }
}
```

`Counter` 类在一个名为 `count` 的变量属性中存储当前值。`Counter` 类还定义了一个名为 `increment` 的方法，每次调用该方法时都会增加 `count` 属性。

`increment()` 方法首先尝试通过查找其数据源上 `increment(forCount:)` 方法的实现来获取增量。`increment()` 方法使用可选链尝试调用 `increment(forCount:)`，并把当前的 `count` 值作为该方法唯一的实参传入。

注意，这里涉及*两*层可选链。首先，`dataSource` 可能为 `nil`，所以 `dataSource` 的名字后面有一个问号，表示只有在 `dataSource` 不为 `nil` 时才应调用 `increment(forCount:)`。其次，即使 `dataSource` *确实*存在，也不能保证它实现了 `increment(forCount:)`，因为这是一个可选要求。在这里，`increment(forCount:)` 可能未实现的情况也由可选链处理。只有在 `increment(forCount:)` 存在——即不为 `nil`——时，才会调用 `increment(forCount:)`。这就是为什么 `increment(forCount:)` 的名字后面也要写一个问号。

由于对 `increment(forCount:)` 的调用可能因上述任一原因失败，所以该调用返回一个*可选*的 `Int` 值。尽管在 `CounterDataSource` 的定义中 `increment(forCount:)` 被定义为返回非可选 `Int` 值，情况仍然如此。即使有两个接连发生的可选链操作，结果仍然只包装在一个可选值中。关于使用多个可选链操作的更多信息，参见 [链接多级可选链](optional-chaining.md#Linking-Multiple-Levels-of-Chaining)。

调用 `increment(forCount:)` 之后，它返回的可选 `Int` 会通过可选绑定解包到一个名为 `amount` 的常量中。如果这个可选 `Int` 确实包含值——即委托对象和方法都存在，并且方法返回了值——就把解包后的 `amount` 加到存储属性 `count` 上，递增就完成了。

如果*无法*从 `increment(forCount:)` 方法获取值——要么因为 `dataSource` 为 nil，要么因为数据源没有实现 `increment(forCount:)`——那么 `increment()` 方法会改为尝试从数据源的 `fixedIncrement` 属性获取值。`fixedIncrement` 也是一个可选要求，所以它的值是可选 `Int` 值，尽管在 `CounterDataSource` 协议定义中 `fixedIncrement` 被定义为非可选 `Int` 属性。

下面是一个简单的 `CounterDataSource` 实现，其中数据源每次被查询时都返回常量 `3`。它通过实现可选的 `fixedIncrement` 属性要求来做到这一点：

```swift
class ThreeSource: NSObject, CounterDataSource {
    let fixedIncrement = 3
}
```

你可以把 `ThreeSource` 的实例用作新的 `Counter` 实例的数据源：

```swift
var counter = Counter()
counter.dataSource = ThreeSource()
for _ in 1...4 {
    counter.increment()
    print(counter.count)
}
// 3
// 6
// 9
// 12
```

上面的代码创建了一个新的 `Counter` 实例；把它的数据源设置为一个新的 `ThreeSource` 实例；并调用计数器的 `increment()` 方法四次。正如预期，每次调用 `increment()` 时，计数器的 `count` 属性都增加 3。

下面是一个更复杂的数据源，名为 `TowardsZeroSource`，它让 `Counter` 实例从当前的 `count` 值向上或向下计数，直到零：

```swift
class TowardsZeroSource: NSObject, CounterDataSource {
    func increment(forCount count: Int) -> Int {
        if count == 0 {
            return 0
        } else if count < 0 {
            return 1
        } else {
            return -1
        }
    }
}
```

`TowardsZeroSource` 类实现了 `CounterDataSource` 协议中可选的 `increment(forCount:)` 方法，并使用 `count` 实参的值判断计数方向。如果 `count` 已经为零，该方法返回 `0`，表示不应再继续计数。

你可以把 `TowardsZeroSource` 的实例与已有的 `Counter` 实例一起使用，从 `-4` 计数到零。一旦计数器到达零，就不再继续计数：

```swift
counter.count = -4
counter.dataSource = TowardsZeroSource()
for _ in 1...5 {
    counter.increment()
    print(counter.count)
}
// -3
// -2
// -1
// 0
// 0
```

## 协议扩展 {#Protocol-Extensions}

协议可以被扩展，从而为遵循类型提供方法、构造器、下标和计算属性的实现。这让你可以在协议本身上定义行为，而不必在每个类型各自的一致性中或全局函数中定义。

例如，可以扩展 `RandomNumberGenerator` 协议来提供一个 `randomBool()` 方法，该方法使用必需的 `random()` 方法的结果来返回一个随机 `Bool` 值：

```swift
extension RandomNumberGenerator {
    func randomBool() -> Bool {
        return random() > 0.5
    }
}
```

通过在协议上创建扩展，所有遵循类型都会自动获得这个方法的实现，无需任何额外修改。

```swift
let generator = LinearCongruentialGenerator()
print("Here's a random number: \(generator.random())")
// Prints "Here's a random number: 0.3746499199817101".
print("And here's a random Boolean: \(generator.randomBool())")
// Prints "And here's a random Boolean: true".
```

协议扩展可以为遵循类型添加实现，但不能让一个协议扩展或继承另一个协议。协议继承始终在协议声明本身中指定。

### 提供默认实现 {#Providing-Default-Implementations}

你可以使用协议扩展为该协议的任何方法或计算属性要求提供默认实现。如果遵循类型为某个必需方法或属性提供了自己的实现，就会使用该实现，而不是扩展提供的实现。

> 注意：带有扩展所提供默认实现的协议要求，与可选协议要求不同。虽然遵循类型都不必为两者提供自己的实现，但带默认实现的要求可以在不使用可选链的情况下调用。

例如，继承了 `TextRepresentable` 协议的 `PrettyTextRepresentable` 协议，可以为其必需的 `prettyTextualDescription` 属性提供默认实现，直接返回访问 `textualDescription` 属性的结果：

```swift
extension PrettyTextRepresentable  {
    var prettyTextualDescription: String {
        return textualDescription
    }
}
```

### 为协议扩展添加约束 {#Adding-Constraints-to-Protocol-Extensions}

定义协议扩展时，你可以指定遵循类型必须满足的约束，满足之后才能使用该扩展的方法和属性。这些约束写在你要扩展的协议名之后，通过编写泛型 `where` 子句来实现。关于泛型 `where` 子句的更多信息，参见 [泛型 where 子句](generics.md#Generic-Where-Clauses)。

例如，你可以为 `Collection` 协议定义一个扩展，让它适用于元素遵循 `Equatable` 协议的任何集合。通过把集合的元素约束为 `Equatable` 协议（Swift 标准库的一部分），你可以使用 `==` 和 `!=` 运算符检查两个元素是否相等或不等。

```swift
extension Collection where Element: Equatable {
    func allEqual() -> Bool {
        for element in self {
            if element != self.first {
                return false
            }
        }
        return true
    }
}
```

只有当集合中的所有元素都相等时，`allEqual()` 方法才返回 `true`。

考虑两个整数数组，一个的所有元素都相同，另一个则不然：

```swift
let equalNumbers = [100, 100, 100, 100, 100]
let differentNumbers = [100, 100, 200, 100, 200]
```

因为数组遵循 `Collection`，整数遵循 `Equatable`，所以 `equalNumbers` 和 `differentNumbers` 可以使用 `allEqual()` 方法：

```swift
print(equalNumbers.allEqual())
// Prints "true".
print(differentNumbers.allEqual())
// Prints "false".
```

> 注意：如果遵循类型满足了多个带约束扩展的要求，而这些扩展为同一个方法或属性提供实现，Swift 会使用与最具体约束对应的实现。
