# 析构过程 {#Deinitialization}

释放需要自定义清理的资源。

*析构器*（deinitializer）会在类实例被释放之前立即调用。你用 `deinit` 关键字编写析构器，类似于用 `init` 关键字编写构造器。析构器只适用于类类型。

## 析构过程如何工作 {#How-Deinitialization-Works}

Swift 会在你的实例不再需要时自动释放它们，以腾出资源。Swift 通过*自动引用计数*（*ARC*）来管理实例的内存，详见 [自动引用计数](automatic-reference-counting.md)。通常情况下，实例被释放时你不需要执行手动清理。然而，当你处理自己的资源时，可能需要自己执行一些额外的清理。例如，如果你创建一个自定义类来打开文件并向其中写入一些数据，你可能需要在类实例被释放之前关闭该文件。

类定义中，每个类最多只能有一个析构器。析构器不接受任何参数，书写时不带括号：

```swift
deinit {
    // perform the deinitialization
}
```

析构器会在实例释放之前自动调用。你不能自己调用析构器。父类的析构器由子类继承，并且父类析构器会在子类析构器实现结束时自动调用。即使子类没有提供自己的析构器，父类析构器也总会被调用。

因为实例要等到其析构器被调用之后才会被释放，所以析构器可以访问正在被析构的那个实例的所有属性，并可以基于这些属性修改自身的行为（例如查找需要关闭的文件的名称）。

## 析构器实战 {#Deinitializers-in-Action}

下面是一个演示析构器如何工作的例子。这个例子为一个简单的游戏定义了两个新类型：`Bank` 和 `Player`。`Bank` 类管理一种虚构的货币，流通中的硬币数量永远不能超过 10,000 枚。游戏中始终只能有一个 `Bank`，因此 `Bank` 被实现为一个带有类型属性和方法的类，用来存储和管理其当前状态：

```swift
class Bank {
    static var coinsInBank = 10_000
    static func distribute(coins numberOfCoinsRequested: Int) -> Int {
        let numberOfCoinsToVend = min(numberOfCoinsRequested, coinsInBank)
        coinsInBank -= numberOfCoinsToVend
        return numberOfCoinsToVend
    }
    static func receive(coins: Int) {
        coinsInBank += coins
    }
}
```

`Bank` 用它的 `coinsInBank` 属性跟踪当前持有的硬币数量。它还提供两个方法——`distribute(coins:)` 和 `receive(coins:)`——来处理硬币的发放和回收。

`distribute(coins:)` 方法在发放硬币之前会检查银行里是否有足够的硬币。如果硬币不够，`Bank` 会返回一个比请求数量更小的数字（如果银行里没有硬币了，就返回零）。它返回一个整数值，表示实际提供的硬币数量。

`receive(coins:)` 方法只是把收到的硬币数量加回银行的硬币储备中。

`Player` 类描述游戏中的一名玩家。每个玩家在任何时候钱包里都存有一定数量的硬币。这由玩家的 `coinsInPurse` 属性表示：

```swift
class Player {
    var coinsInPurse: Int
    init(coins: Int) {
        coinsInPurse = Bank.distribute(coins: coins)
    }
    func win(coins: Int) {
        coinsInPurse += Bank.distribute(coins: coins)
    }
    deinit {
        Bank.receive(coins: coinsInPurse)
    }
}
```

每个 `Player` 实例在构造过程中都会从银行获得指定数量的硬币作为初始额度，不过如果可用硬币不够，`Player` 实例拿到的可能会比这个数量少。

`Player` 类定义了一个 `win(coins:)` 方法，它从银行取出一定数量的硬币并加入玩家的钱包。`Player` 类还实现了一个析构器，它会在 `Player` 实例被释放之前立即被调用。在这里，析构器只是把玩家的所有硬币都归还给银行：

```swift
var playerOne: Player? = Player(coins: 100)
print("A new player has joined the game with \(playerOne!.coinsInPurse) coins")
// Prints "A new player has joined the game with 100 coins".
print("There are now \(Bank.coinsInBank) coins left in the bank")
// Prints "There are now 9900 coins left in the bank".
```

创建了一个新的 `Player` 实例，请求 100 枚硬币（如果有的话）。这个 `Player` 实例存储在一个名为 `playerOne` 的可选 `Player` 变量中。这里使用可选变量，是因为玩家随时可能离开游戏。可选值让你能够跟踪当前游戏中是否有玩家。

由于 `playerOne` 是可选值，所以在访问它的 `coinsInPurse` 属性以打印其初始硬币数量时，以及在调用它的 `win(coins:)` 方法时，都要用感叹号（`!`）来限定它：

```swift
playerOne!.win(coins: 2_000)
print("PlayerOne won 2000 coins & now has \(playerOne!.coinsInPurse) coins")
// Prints "PlayerOne won 2000 coins & now has 2100 coins".
print("The bank now only has \(Bank.coinsInBank) coins left")
// Prints "The bank now only has 7900 coins left".
```

这里，玩家赢了 2,000 枚硬币。玩家的钱包现在有 2,100 枚硬币，而银行只剩下 7,900 枚硬币。

```swift
playerOne = nil
print("PlayerOne has left the game")
// Prints "PlayerOne has left the game".
print("The bank now has \(Bank.coinsInBank) coins")
// Prints "The bank now has 10000 coins".
```

玩家现在离开了游戏。这是通过把可选变量 `playerOne` 设为 `nil` 来表示的，意思是“没有 `Player` 实例”。此时，`playerOne` 变量对 `Player` 实例的引用就断开了。没有其他属性或变量还在引用这个 `Player` 实例，所以它被释放以腾出内存。就在这发生之前，它的析构器被自动调用，它的硬币也归还给了银行。
