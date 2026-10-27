# 嵌套类型 {#Nested-Types}

在另一个类型的作用域内定义类型。

枚举常常是为了支持某个特定类或结构体的功能而创建的。同样，有时把工具性结构体纯粹定义为供某个更复杂的类型在内部使用，以及把通常与某个特定类型配合使用的协议也这样定义，会很方便。为此，Swift 允许你定义*嵌套类型*（nested type），把枚举、结构体和协议这类辅助类型嵌套到它们所支持类型的定义内部。

要把一个类型嵌套到另一个类型内部，只需把它的定义写在其所支持类型的外层大括号内。类型可以按需要嵌套任意多层。

## 嵌套类型的实际运用 {#Nested-Types-in-Action}

下面的例子定义了一个名为 `BlackjackCard` 的结构体，用来为 21 点（Blackjack）游戏中使用的扑克牌建模。`BlackjackCard` 结构体包含两个嵌套的枚举类型，分别名为 `Suit` 和 `Rank`。

在 21 点中，A（Ace）牌的点数可以是 1，也可以是 11。这一特性由嵌套在 `Rank` 枚举中的 `Values` 结构体来表示：

```swift
struct BlackjackCard {
    // nested Suit enumeration
    enum Suit: Character {
        case spades = "♠", hearts = "♡", diamonds = "♢", clubs = "♣"
    }

    // nested Rank enumeration
    enum Rank: Int {
        case two = 2, three, four, five, six, seven, eight, nine, ten
        case jack, queen, king, ace
        struct Values {
            let first: Int, second: Int?
        }
        var values: Values {
            switch self {
            case .ace:
                return Values(first: 1, second: 11)
            case .jack, .queen, .king:
                return Values(first: 10, second: nil)
            default:
                return Values(first: self.rawValue, second: nil)
            }
        }
    }

    // BlackjackCard properties and methods
    let rank: Rank, suit: Suit
    var description: String {
        var output = "suit is \(suit.rawValue),"
        output += " value is \(rank.values.first)"
        if let second = rank.values.second {
            output += " or \(second)"
        }
        return output
    }
}
```

`Suit` 枚举描述了扑克牌常见的四种花色，并用原始的 `Character` 值来表示花色符号。

`Rank` 枚举描述了扑克牌可能的十三种点数，并用原始的 `Int` 值来表示它们的牌面值。（这个原始 `Int` 值并不用于 J、Q、K 和 A 这几张牌。）

如上所述，`Rank` 枚举还定义了自己的另一个嵌套结构体，名为 `Values`。这个结构体封装了这样一个事实：大多数牌只有一个值，而 A 有两个值。`Values` 结构体定义了两个属性来表示这一点：

- `first`，类型为 `Int`
- `second`，类型为 `Int?`，即“可选 `Int`”

`Rank` 还定义了一个计算属性 `values`，它返回 `Values` 结构体的一个实例。这个计算属性会考虑牌的点数，并根据点数用合适的值初始化一个新的 `Values` 实例。对于 `jack`、`queen`、`king` 和 `ace`，它使用特殊的值；对于数字牌，它使用该点数的原始 `Int` 值。

`BlackjackCard` 结构体本身有两个属性——`rank` 和 `suit`。它还定义了一个名为 `description` 的计算属性，该属性使用 `rank` 和 `suit` 中存储的值来构建对这张牌的名称和值的描述。`description` 属性使用可选绑定来检查是否有第二个值需要显示，如果有，就为第二个值插入额外的描述细节。

因为 `BlackjackCard` 是一个没有自定义构造器的结构体，所以它有一个隐式的成员逐一构造器，详见 [结构体类型的成员逐一构造器](initialization.md#Memberwise-Initializers-for-Structure-Types)。你可以用这个构造器来初始化一个新的常量 `theAceOfSpades`：

```swift
let theAceOfSpades = BlackjackCard(rank: .ace, suit: .spades)
print("theAceOfSpades: \(theAceOfSpades.description)")
// Prints "theAceOfSpades: suit is ♠, value is 1 or 11".
```

尽管 `Rank` 和 `Suit` 嵌套在 `BlackjackCard` 内部，但它们的类型可以从上下文中推断出来，因此这个实例的初始化可以只通过成员名（`.ace` 和 `.spades`）来引用枚举成员。在上面的例子中，`description` 属性正确地报告了黑桃 A 的值为 `1` 或 `11`。

## 引用嵌套类型 {#Referring-to-Nested-Types}

要在定义上下文之外使用嵌套类型，需要在其名称前面加上它所在外层类型的名称：

```swift
let heartsSymbol = BlackjackCard.Suit.hearts.rawValue
// heartsSymbol is "♡"
```

对于上面的例子，这样做可以让 `Suit`、`Rank` 和 `Values` 的名称保持得刻意简短，因为这些名称会自然地由它们所处的定义上下文来限定。
