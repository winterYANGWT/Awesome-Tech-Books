# 类型转换 {#Type-Casting}

确定值的运行时类型，并为其提供更具体的类型信息。

*类型转换*（type casting）是一种检查实例类型的方式，也可以把该实例当作其自身类层次结构中其他位置的另一个父类或子类来对待。

Swift 中的类型转换通过 `is` 和 `as` 运算符实现。这两个运算符提供了一种简单而富有表现力的方式，用来检查值的类型，或把值转换成另一种类型。

你还可以使用类型转换来检查某个类型是否遵循协议，详见 [检查协议一致性](protocols.md#Checking-for-Protocol-Conformance)。

## 为类型转换定义类层次结构 {#Defining-a-Class-Hierarchy-for-Type-Casting}

你可以对类和子类的层次结构使用类型转换，来检查某个特定类实例的类型，并把该实例转换成同一层次结构中的另一个类。下面的三段代码定义了一个类层次结构，以及一个包含这些类实例的数组，用于演示类型转换。

第一段代码定义了一个名为 `MediaItem` 的新基类。这个类为数字媒体库中出现的任何种类的条目提供基本功能。具体来说，它声明了一个 `String` 类型的 `name` 属性，以及一个 `init(name:)` 构造器。（这里假设所有媒体条目，包括所有电影和歌曲，都有一个名称。）

```swift
class MediaItem {
    var name: String
    init(name: String) {
        self.name = name
    }
}
```

下一段代码定义了 `MediaItem` 的两个子类。第一个子类 `Movie` 封装了关于一部电影或影片的额外信息。它在基类 `MediaItem` 的基础上添加了一个 `director` 属性，并提供了相应的构造器。第二个子类 `Song` 在基类的基础上添加了 `artist` 属性和构造器：

```swift
class Movie: MediaItem {
    var director: String
    init(name: String, director: String) {
        self.director = director
        super.init(name: name)
    }
}

class Song: MediaItem {
    var artist: String
    init(name: String, artist: String) {
        self.artist = artist
        super.init(name: name)
    }
}
```

最后一段代码创建了一个名为 `library` 的常量数组，其中包含两个 `Movie` 实例和三个 `Song` 实例。`library` 数组的类型是根据初始化它的数组字面量内容推断出来的。Swift 的类型检查器能够推断出 `Movie` 和 `Song` 有一个共同的父类 `MediaItem`，因此它为 `library` 数组推断出 `[MediaItem]` 类型：

```swift
let library = [
    Movie(name: "Casablanca", director: "Michael Curtiz"),
    Song(name: "Blue Suede Shoes", artist: "Elvis Presley"),
    Movie(name: "Citizen Kane", director: "Orson Welles"),
    Song(name: "The One And Only", artist: "Chesney Hawkes"),
    Song(name: "Never Gonna Give You Up", artist: "Rick Astley")
]
// the type of "library" is inferred to be [MediaItem]
```

存储在 `library` 中的条目在底层仍然是 `Movie` 和 `Song` 实例。然而，如果你遍历这个数组的内容，取回的条目类型是 `MediaItem`，而不是 `Movie` 或 `Song`。为了把它们当作原本的类型来使用，你需要*检查*它们的类型，或者把它们*向下转型*为另一种类型，如下所述。

## 检查类型 {#Checking-Type}

使用*类型检查运算符*（`is`）来检查某个实例是否属于某个子类类型。如果实例属于该子类类型，类型检查运算符返回 `true`，否则返回 `false`。

下面的例子定义了两个变量 `movieCount` 和 `songCount`，用来统计 `library` 数组中 `Movie` 和 `Song` 实例的数量：

```swift
var movieCount = 0
var songCount = 0

for item in library {
    if item is Movie {
        movieCount += 1
    } else if item is Song {
        songCount += 1
    }
}

print("Media library contains \(movieCount) movies and \(songCount) songs")
// Prints "Media library contains 2 movies and 3 songs".
```

这个例子遍历 `library` 数组中的所有条目。每一轮循环中，`for`-`in` 循环都会把 `item` 常量设置为数组中的下一个 `MediaItem`。

如果当前的 `MediaItem` 是 `Movie` 实例，`item is Movie` 返回 `true`，否则返回 `false`。类似地，`item is Song` 检查该条目是否为 `Song` 实例。在 `for`-`in` 循环结束时，`movieCount` 和 `songCount` 的值就分别是每种类型找到的 `MediaItem` 实例数量。

## 向下转型 {#Downcasting}

某个类类型的常量或变量，在底层引用的可能实际上是其子类的实例。如果你确信情况如此，可以尝试用*类型转换运算符*（`as?` 或 `as!`）*向下转型*到子类类型。

因为向下转型可能失败，类型转换运算符有两种不同的形式。条件形式 `as?` 返回一个可选值，其类型是你尝试向下转型到的类型。强制形式 `as!` 会尝试向下转型，并把结果强制解包，整个过程作为一个复合操作完成。

当你不确定向下转型是否会成功时，请使用类型转换运算符的条件形式（`as?`）。这种形式的运算符总是返回一个可选值；如果无法向下转型，该值就是 `nil`。这让你能够检查向下转型是否成功。

只有当你确定向下转型总会成功时，才使用类型转换运算符的强制形式（`as!`）。如果你尝试向下转型到错误的类类型，这种形式的运算符会触发运行时错误。

下面的例子遍历 `library` 中的每个 `MediaItem`，并为每个条目打印合适的描述。为此，它需要把每个条目当作真正的 `Movie` 或 `Song` 来访问，而不只是当作 `MediaItem`。这样才能访问 `Movie` 或 `Song` 的 `director` 或 `artist` 属性，用于生成描述。

在这个例子中，数组里的每个条目可能是一个 `Movie`，也可能是一个 `Song`。你无法预先知道每个条目实际使用哪个类，因此适合使用类型转换运算符的条件形式（`as?`），在每一轮循环中检查向下转型：

```swift
for item in library {
    if let movie = item as? Movie {
        print("Movie: \(movie.name), dir. \(movie.director)")
    } else if let song = item as? Song {
        print("Song: \(song.name), by \(song.artist)")
    }
}

// Movie: Casablanca, dir. Michael Curtiz
// Song: Blue Suede Shoes, by Elvis Presley
// Movie: Citizen Kane, dir. Orson Welles
// Song: The One And Only, by Chesney Hawkes
// Song: Never Gonna Give You Up, by Rick Astley
```

这个例子首先尝试把当前的 `item` 向下转型为 `Movie`。因为 `item` 是 `MediaItem` 实例，所以它*可能*是 `Movie`；同样，它也可能是 `Song`，或者只是一个基类 `MediaItem`。由于这种不确定性，当尝试向下转型到子类类型时，`as?` 形式的类型转换运算符会返回一个*可选*值。`item as? Movie` 的结果是 `Movie?` 类型，也就是“可选 `Movie`”。

把 library 数组中的 `Song` 实例向下转型为 `Movie` 会失败。为了处理这种情况，上面的例子使用可选绑定来检查这个可选 `Movie` 是否实际包含值（也就是说，查明向下转型是否成功）。这个可选绑定写作“`if let movie = item as? Movie`”，可以读作：

“尝试把 `item` 作为 `Movie` 来访问。如果成功，就把返回的可选 `Movie` 中存储的值赋给一个名为 `movie` 的新临时常量。”

如果向下转型成功，随后就会使用 `movie` 的属性来打印该 `Movie` 实例的描述，包括其 `director` 的名字。检查 `Song` 实例时也使用类似的原理，只要在库中找到 `Song`，就打印合适的描述（包括 `artist` 的名字）。

> 注意：类型转换实际上不会修改实例，也不会改变它的值。底层实例保持不变；只是把它当作转换后的类型实例来对待和访问。

## Any 和 AnyObject 的类型转换 {#Type-Casting-for-Any-and-AnyObject}

Swift 提供了两种特殊类型来处理非特定类型：

- `Any` 可以表示任何类型的实例，包括函数类型。
- `AnyObject` 可以表示任何类类型的实例。

只有当你明确需要 `Any` 和 `AnyObject` 提供的行为和能力时，才使用它们。在代码中明确你预期使用的类型总是更好的做法。

下面是一个使用 `Any` 来处理不同类型混合值的例子，其中包括函数类型和非类类型。这个例子创建了一个名为 `things` 的数组，它可以存储 `Any` 类型的值：

```swift
var things: [Any] = []

things.append(0)
things.append(0.0)
things.append(42)
things.append(3.14159)
things.append("hello")
things.append((3.0, 5.0))
things.append(Movie(name: "Ghostbusters", director: "Ivan Reitman"))
things.append({ (name: String) -> String in "Hello, \(name)" })
```

`things` 数组包含两个 `Int` 值、两个 `Double` 值、一个 `String` 值、一个 `(Double, Double)` 类型的元组、电影“Ghostbusters”，以及一个接受 `String` 值并返回另一个 `String` 值的闭包表达式。

要查明一个只知道是 `Any` 或 `AnyObject` 类型的常量或变量的具体类型，你可以在 `switch` 语句的分支中使用 `is` 或 `as` 模式。下面的例子遍历 `things` 数组中的条目，并用 `switch` 语句查询每个条目的类型。`switch` 语句中有几个分支会把匹配到的值绑定到指定类型的常量，以便打印它的值：

```swift
for thing in things {
    switch thing {
    case 0 as Int:
        print("zero as an Int")
    case 0 as Double:
        print("zero as a Double")
    case let someInt as Int:
        print("an integer value of \(someInt)")
    case let someDouble as Double where someDouble > 0:
        print("a positive double value of \(someDouble)")
    case is Double:
        print("some other double value that I don't want to print")
    case let someString as String:
        print("a string value of \"\(someString)\"")
    case let (x, y) as (Double, Double):
        print("an (x, y) point at \(x), \(y)")
    case let movie as Movie:
        print("a movie called \(movie.name), dir. \(movie.director)")
    case let stringConverter as (String) -> String:
        print(stringConverter("Michael"))
    default:
        print("something else")
    }
}

// zero as an Int
// zero as a Double
// an integer value of 42
// a positive double value of 3.14159
// a string value of "hello"
// an (x, y) point at 3.0, 5.0
// a movie called Ghostbusters, dir. Ivan Reitman
// Hello, Michael
```

> 注意：`Any` 类型可以表示任何类型的值，包括可选类型。如果你在需要 `Any` 类型值的地方使用可选值，Swift 会给出警告。如果你确实需要把可选值作为 `Any` 值使用，可以使用 `as` 运算符把可选值显式转换成 `Any`，如下所示。
>
> ```swift
> let optionalNumber: Int? = 3
> things.append(optionalNumber)        // Warning
> things.append(optionalNumber as Any) // No warning
> ```
