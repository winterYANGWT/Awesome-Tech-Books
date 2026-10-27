# Swift 概览 {#A-Swift-Tour}

探索 Swift 的功能和语法。

传统上，一门新语言的第一个程序应该在屏幕上打印“Hello, world!”。在 Swift 中，这可以用一行代码完成：

```swift
print("Hello, world!")
// Prints "Hello, world!"
```

如果你了解其他语言，这个语法应该看起来很熟悉——在 Swift 中，这一行代码就是一个完整的程序。你不需要为输出文本或处理字符串之类的功能导入单独的库。写在全局作用域的代码会被用作程序的入口点，所以你不需要 `main()` 函数。你也不需要在每条语句末尾写分号。

这次概览通过向你展示如何完成各种编程任务，提供了足够的信息，让你可以开始用 Swift 编写代码。如果有不明白的地方，不用担心——本次概览中介绍的所有内容都会在本书其余部分详细解释。

## 简单值 {#Simple-Values}

使用 `let` 创建常量，使用 `var` 创建变量。常量的值不必在编译时确定，但你必须恰好给它赋值一次。这意味着你可以用常量来为一个只确定一次、却在多处使用的值命名。

```swift
var myVariable = 42
myVariable = 50
let myConstant = 42
```

常量或变量的类型必须与你要赋给它的值的类型相同。不过，你不必总是显式写出类型。在创建常量或变量时提供一个值，编译器就能推断出它的类型。在上面的例子中，编译器推断 `myVariable` 是整数，因为它的初始值是整数。

如果初始值没有提供足够的信息（或者没有初始值），请在变量名后面写上类型并用冒号分隔，以此指定类型。

```swift
let implicitInteger = 70
let implicitDouble = 70.0
let explicitDouble: Double = 70
```

> 练习：创建一个显式类型为 `Float`、值为 `4` 的常量。

值永远不会被隐式转换为其他类型。如果你需要把值转换为其他类型，请显式创建所需类型的实例。

```swift
let label = "The width is "
let width = 94
let widthLabel = label + String(width)
```

> 练习：试着删掉最后一行中对 `String` 的转换。你会得到什么错误？

把值包含在字符串中还有一种更简单的方法：把值写在括号里，并在括号前写一个反斜杠（`\`）。例如：

```swift
let apples = 3
let oranges = 5
let appleSummary = "I have \(apples) apples."
let fruitSummary = "I have \(apples + oranges) pieces of fruit."
```

> 练习：用 `\()` 在字符串中包含一个浮点数计算，并在问候语中包含某人的名字。

对于跨越多行的字符串，使用三个双引号（`"""`）。只要每行开头的缩进与结尾引号的缩进一致，每行开头的缩进就会被移除。例如：

```swift
let quotation = """
        Even though there's whitespace to the left,
        the actual lines aren't indented.
            Except for this line.
        Double quotes (") can appear without being escaped.

        I still have \(apples + oranges) pieces of fruit.
        """
```

使用方括号（`[]`）创建数组和字典，并通过在方括号中写索引或键来访问其中的元素。最后一个元素后面允许有逗号。

```swift
var fruits = ["strawberries", "limes", "tangerines"]
fruits[1] = "grapes"

var occupations = [
    "Malcolm": "Captain",
    "Kaylee": "Mechanic",
 ]
occupations["Jayne"] = "Public Relations"
```

数组会在你添加元素时自动增长。

```swift
fruits.append("blueberries")
print(fruits)
// Prints "["strawberries", "grapes", "tangerines", "blueberries"]".
```

你也可以用方括号来写空数组或空字典。对于数组，写 `[]`；对于字典，写 `[:]`。

```swift
fruits = []
occupations = [:]
```

如果你要把空数组或空字典赋给一个新变量，或者赋给其他没有任何类型信息的地方，就需要指定类型。

```swift
let emptyArray: [String] = []
let emptyDictionary: [String: Float] = [:]
```

## 控制流 {#Control-Flow}

使用 `if` 和 `switch` 创建条件分支，使用 `for`-`in`、`while` 和 `repeat`-`while` 创建循环。条件或循环变量外面的括号是可选的。循环体外面的大括号是必需的。

```swift
let individualScores = [75, 43, 103, 87, 12]
var teamScore = 0
for score in individualScores {
    if score > 50 {
        teamScore += 3
    } else {
        teamScore += 1
    }
}
print(teamScore)
// Prints "11".
```

在 `if` 语句中，条件必须是布尔表达式——这意味着像 `if score { ... }` 这样的代码是错误的，它不是隐式地与零作比较。

你可以在赋值语句的等号（`=`）后面，或者在 `return` 后面写 `if` 或 `switch`，根据条件来选择一个值。

```swift
let scoreDecoration = if teamScore > 10 {
    "🎉"
} else {
    ""
}
print("Score:", teamScore, scoreDecoration)
// Prints "Score: 11 🎉".
```

你可以把 `if` 和 `let` 一起使用，来处理可能缺失的值。这些值用可选值表示。可选值要么包含一个值，要么包含 `nil`，表示值缺失。在值的类型后面写一个问号（`?`），就把这个值标记为可选。

```swift
var optionalString: String? = "Hello"
print(optionalString == nil)
// Prints "false".

var optionalName: String? = "John Appleseed"
var greeting = "Hello!"
if let name = optionalName {
    greeting = "Hello, \(name)"
}
print(greeting)
```

> 练习：把 `optionalName` 改成 `nil`。你会得到什么问候语？添加一个 `else` 子句，在 `optionalName` 为 `nil` 时设置另一条问候语。

如果可选值为 `nil`，条件就是 `false`，大括号里的代码会被跳过。否则，可选值会被解包并赋给 `let` 后面的常量，这样解包后的值就可以在代码块内使用。

处理可选值的另一种方法是使用 `??` 运算符提供一个默认值。如果可选值缺失，就改为使用默认值。

```swift
let nickname: String? = nil
let fullName: String = "John Appleseed"
let informalGreeting = "Hi \(nickname ?? fullName)"
```

你可以用更简短的写法来解包一个值，让解包后的值使用相同的名字。

```swift
if let nickname {
    print("Hey, \(nickname)")
}
// Doesn't print anything, because nickname is nil.
```

`switch` 支持任何类型的数据和各种各样的比较操作——它并不局限于整数和相等性判断。

```swift
let vegetable = "red pepper"
switch vegetable {
case "celery":
    print("Add some raisins and make ants on a log.")
case "cucumber", "watercress":
    print("That would make a good tea sandwich.")
case let x where x.hasSuffix("pepper"):
    print("Is it a spicy \(x)?")
default:
    print("Everything tastes good in soup.")
}
// Prints "Is it a spicy red pepper?"
```

> 练习：试着删掉 `default` 分支。你会得到什么错误？

注意，`let` 可以用在模式中，把与模式匹配的值赋给一个常量。

在执行完匹配的 `switch` 分支中的代码后，程序会退出 `switch` 语句。执行不会继续进入下一个分支，因此你不需要在每个分支代码的末尾显式跳出 `switch`。

你使用 `for`-`in` 遍历字典中的项，为每一组键值对提供一对名字来分别接收键和值。字典是无序集合，因此其键和值按任意顺序被遍历。

```swift
let interestingNumbers = [
    "Prime": [2, 3, 5, 7, 11, 13],
    "Fibonacci": [1, 1, 2, 3, 5, 8],
    "Square": [1, 4, 9, 16, 25],
]
var largest = 0
for (_, numbers) in interestingNumbers {
    for number in numbers {
        if number > largest {
            largest = number
        }
    }
}
print(largest)
// Prints "25".
```

> 练习：用某个变量名替换 `_`，并记录哪种数最大。

使用 `while` 重复执行一段代码，直到条件发生变化。循环的条件也可以放在末尾，确保循环至少执行一次。

```swift
var n = 2
while n < 100 {
    n *= 2
}
print(n)
// Prints "128".

var m = 2
repeat {
    m *= 2
} while m < 100
print(m)
// Prints "128".
```

> 练习：把条件从 `m < 100` 改成 `m < 0`，看看当循环条件一开始就为假时，`while` 和 `repeat`-`while` 的行为有何不同。

你可以在循环中使用 `..<` 创建索引区间，从而遍历索引。

```swift
var total = 0
for i in 0..<4 {
    total += i
}
print(total)
// Prints "6".
```

使用 `..<` 创建不包含上界值的区间，使用 `...` 创建包含两个端点的区间。

## 函数和闭包 {#Functions-and-Closures}

使用 `func` 声明函数。调用函数时，在函数名后面跟一对括号，括号里是实参列表。使用 `->` 把形参名和类型与函数的返回类型分隔开。

```swift
func greet(person: String, day: String) -> String {
    return "Hello \(person), today is \(day)."
}
greet(person: "Bob", day: "Tuesday")
```

> 练习：删掉 `day` 形参。添加一个形参，在问候语中包含今天的午餐特供。

默认情况下，函数把形参名用作实参标签。在形参名前面写一个自定义的实参标签，或者写 `_` 来不使用实参标签。

```swift
func greet(_ person: String, on day: String) -> String {
    return "Hello \(person), today is \(day)."
}
greet("John", on: "Wednesday")
```

使用元组创建一个复合值——例如，从函数返回多个值。元组的元素既可以通过名字引用，也可以通过编号引用。

```swift
func calculateStatistics(scores: [Int]) -> (min: Int, max: Int, sum: Int) {
    var min = scores[0]
    var max = scores[0]
    var sum = 0

    for score in scores {
        if score > max {
            max = score
        } else if score < min {
            min = score
        }
        sum += score
    }

    return (min, max, sum)
}
let statistics = calculateStatistics(scores: [5, 3, 100, 3, 9])
print(statistics.sum)
// Prints "120".
print(statistics.2)
// Prints "120".
```

函数可以嵌套。嵌套函数可以访问在外层函数中声明的变量。你可以用嵌套函数来组织长函数或复杂函数中的代码。

```swift
func returnFifteen() -> Int {
    var y = 10
    func add() {
        y += 5
    }
    add()
    return y
}
returnFifteen()
```

函数是一等类型。这意味着函数可以把另一个函数作为它的值返回。

```swift
func makeIncrementer() -> ((Int) -> Int) {
    func addOne(number: Int) -> Int {
        return 1 + number
    }
    return addOne
}
var increment = makeIncrementer()
increment(7)
```

函数可以把另一个函数作为它的一个实参。

```swift
func hasAnyMatches(list: [Int], condition: (Int) -> Bool) -> Bool {
    for item in list {
        if condition(item) {
            return true
        }
    }
    return false
}
func lessThanTen(number: Int) -> Bool {
    return number < 10
}
var numbers = [20, 19, 7, 12]
hasAnyMatches(list: numbers, condition: lessThanTen)
```

函数实际上是闭包的一种特例：闭包是可以在之后被调用的代码块。闭包中的代码可以访问创建闭包的作用域中可用的变量和函数等内容，即使闭包在执行时处于不同的作用域——你在嵌套函数中已经见过这样的例子。你可以用大括号（`{}`）把代码括起来，写成没有名字的闭包。使用 `in` 把实参和返回类型与闭包体分隔开。

```swift
numbers.map({ (number: Int) -> Int in
    let result = 3 * number
    return result
})
```

> 练习：重写这个闭包，让所有奇数都返回零。

你有几种方法可以更简洁地编写闭包。当闭包的类型已知时，例如委托的回调，你可以省略形参的类型、返回类型，或者两者都省略。单语句闭包会隐式返回其唯一语句的值。

```swift
let mappedNumbers = numbers.map({ number in 3 * number })
print(mappedNumbers)
// Prints "[60, 57, 21, 36]".
```

你可以用编号而不是名字来引用形参——这种方法在非常短的闭包中特别有用。作为最后一个实参传给函数的闭包可以紧跟在括号后面。当闭包是函数的唯一实参时，你可以完全省略括号。

```swift
let sortedNumbers = numbers.sorted { $0 > $1 }
print(sortedNumbers)
// Prints "[20, 19, 12, 7]".
```

## 对象和类 {#Objects-and-Classes}

使用 `class` 后跟类名来创建类。类中的属性声明写法与常量或变量声明相同，只不过它处于类的上下文中。同样，方法和函数的声明写法也相同。

```swift
class Shape {
    var numberOfSides = 0
    func simpleDescription() -> String {
        return "A shape with \(numberOfSides) sides."
    }
}
```

> 练习：用 `let` 添加一个常量属性，再添加一个接受实参的方法。

在类名后面加上括号，就可以创建类的实例。使用点语法访问实例的属性和方法。

```swift
var shape = Shape()
shape.numberOfSides = 7
var shapeDescription = shape.simpleDescription()
```

这个版本的 `Shape` 类缺少一样重要的东西：在创建实例时用来设置类的构造器。使用 `init` 创建一个构造器。

```swift
class NamedShape {
    var numberOfSides: Int = 0
    var name: String

    init(name: String) {
       self.name = name
    }

    func simpleDescription() -> String {
       return "A shape with \(numberOfSides) sides."
    }
}
```

注意 `self` 如何用来区分 `name` 属性和构造器的 `name` 实参。创建类的实例时，构造器的实参像函数调用一样传入。每个属性都需要被赋值——要么在声明时赋值（如 `numberOfSides`），要么在构造器中赋值（如 `name`）。

如果你需要在对象被释放之前执行一些清理工作，可以使用 `deinit` 创建一个析构器。

子类在类名后面包含父类的名字，用冒号分隔。类不要求必须继承任何标准根类，因此你可以根据需要包含或省略父类。

子类中重写父类实现的方法用 `override` 标记——意外重写方法而没有写 `override`，编译器会检测为错误。编译器也会检测带有 `override`、但实际上没有重写父类中任何方法的方法。

```swift
class Square: NamedShape {
    var sideLength: Double

    init(sideLength: Double, name: String) {
        self.sideLength = sideLength
        super.init(name: name)
        numberOfSides = 4
    }

    func area() -> Double {
        return sideLength * sideLength
    }

    override func simpleDescription() -> String {
        return "A square with sides of length \(sideLength)."
    }
}
let test = Square(sideLength: 5.2, name: "my test square")
test.area()
test.simpleDescription()
```

> 练习：再写一个 `NamedShape` 的子类，叫做 `Circle`，让它的构造器接受半径和名字作为实参。在 `Circle` 类上实现 `area()` 和 `simpleDescription()` 方法。

除了简单的存储属性，属性还可以有 getter 和 setter。

```swift
class EquilateralTriangle: NamedShape {
    var sideLength: Double = 0.0

    init(sideLength: Double, name: String) {
        self.sideLength = sideLength
        super.init(name: name)
        numberOfSides = 3
    }

    var perimeter: Double {
        get {
             return 3.0 * sideLength
        }
        set {
            sideLength = newValue / 3.0
        }
    }

    override func simpleDescription() -> String {
        return "An equilateral triangle with sides of length \(sideLength)."
    }
}
var triangle = EquilateralTriangle(sideLength: 3.1, name: "a triangle")
print(triangle.perimeter)
// Prints "9.3".
triangle.perimeter = 9.9
print(triangle.sideLength)
// Prints "3.3000000000000003".
```

在 `perimeter` 的 setter 中，新值有一个隐式名称 `newValue`。你可以在 `set` 后面的括号中提供一个显式名称。

注意 `EquilateralTriangle` 类的构造器有三个不同的步骤：

1. 设置子类声明的属性的值。
2. 调用父类的构造器。
3. 修改父类定义的属性的值。任何使用方法、getter 或 setter 完成的额外设置工作也可以在这一步进行。

如果你不需要计算属性值，但仍然需要提供在设置新值前后运行的代码，请使用 `willSet` 和 `didSet`。只要值在构造器之外发生变化，你提供的代码就会运行。例如，下面的类确保其三角形的边长始终与其正方形的边长相同。

```swift
class TriangleAndSquare {
    var triangle: EquilateralTriangle {
        willSet {
            square.sideLength = newValue.sideLength
        }
    }
    var square: Square {
        willSet {
            triangle.sideLength = newValue.sideLength
        }
    }
    init(size: Double, name: String) {
        square = Square(sideLength: size, name: name)
        triangle = EquilateralTriangle(sideLength: size, name: name)
    }
}
var triangleAndSquare = TriangleAndSquare(size: 10, name: "another test shape")
print(triangleAndSquare.square.sideLength)
// Prints "10.0".
print(triangleAndSquare.triangle.sideLength)
// Prints "10.0".
triangleAndSquare.square = Square(sideLength: 50, name: "larger square")
print(triangleAndSquare.triangle.sideLength)
// Prints "50.0".
```

处理可选值时，你可以在方法、属性和下标等操作前面写 `?`。如果 `?` 前面的值为 `nil`，那么 `?` 后面的一切都会被忽略，整个表达式的值为 `nil`。否则，可选值会被解包，`?` 后面的一切都会作用于解包后的值。在这两种情况下，整个表达式的值都是可选值。

```swift
let optionalSquare: Square? = Square(sideLength: 2.5, name: "optional square")
let sideLength = optionalSquare?.sideLength
```

## 枚举和结构体 {#Enumerations-and-Structures}

使用 `enum` 创建枚举。与类以及所有其他具名类型一样，枚举可以拥有自己的方法。

```swift
enum Rank: Int {
    case ace = 1
    case two, three, four, five, six, seven, eight, nine, ten
    case jack, queen, king

    func simpleDescription() -> String {
        switch self {
        case .ace:
            return "ace"
        case .jack:
            return "jack"
        case .queen:
            return "queen"
        case .king:
            return "king"
        default:
            return String(self.rawValue)
        }
    }
}
let ace = Rank.ace
let aceRawValue = ace.rawValue
```

> 练习：写一个函数，通过比较原始值来比较两个 `Rank` 值。

默认情况下，Swift 从零开始分配原始值，每次递增一，但你可以通过显式指定值来改变这种行为。在上面的例子中，`Ace` 被显式赋予原始值 `1`，其余原始值按顺序分配。你也可以使用字符串或浮点数作为枚举的原始类型。使用 `rawValue` 属性访问枚举成员的原始值。

使用 `init?(rawValue:)` 构造器从原始值创建枚举的实例。它返回与原始值匹配的枚举成员，如果没有匹配的 `Rank`，则返回 `nil`。

```swift
if let convertedRank = Rank(rawValue: 3) {
    let threeDescription = convertedRank.simpleDescription()
}
```

枚举的成员值是实际的值，而不仅仅是书写其原始值的另一种方式。事实上，在没有有意义的原始值时，你不必提供原始值。

```swift
enum Suit {
    case spades, hearts, diamonds, clubs

    func simpleDescription() -> String {
        switch self {
        case .spades:
            return "spades"
        case .hearts:
            return "hearts"
        case .diamonds:
            return "diamonds"
        case .clubs:
            return "clubs"
        }
    }
}
let hearts = Suit.hearts
let heartsDescription = hearts.simpleDescription()
```

> 练习：给 `Suit` 添加一个 `color()` 方法，对黑桃和梅花返回“black”，对红心和方块返回“red”。

注意上面引用枚举的 `hearts` 成员的两种方式：给 `hearts` 常量赋值时，枚举成员 `Suit.hearts` 用完整名字引用，因为该常量没有指定显式类型。在 `switch` 内部，枚举成员用缩写形式 `.hearts` 引用，因为 `self` 的值已知是花色。只要值的类型已知，你就可以使用缩写形式。

如果枚举有原始值，这些值在声明时确定，这意味着特定枚举成员的每个实例总是具有相同的原始值。枚举成员的另一种做法是让值与该成员关联——这些值在你创建实例时确定，并且对于枚举成员的每个实例可以不同。你可以把关联值看作枚举成员实例的存储属性。例如，考虑从服务器请求日出和日落时间的场景。服务器要么返回所请求的信息，要么返回出错原因的说明。

```swift
enum ServerResponse {
    case result(String, String)
    case failure(String)
}

let success = ServerResponse.result("6:00 am", "8:09 pm")
let failure = ServerResponse.failure("Out of cheese.")

switch success {
case let .result(sunrise, sunset):
    print("Sunrise is at \(sunrise) and sunset is at \(sunset).")
case let .failure(message):
    print("Failure...  \(message)")
}
// Prints "Sunrise is at 6:00 am and sunset is at 8:09 pm."
```

> 练习：给 `ServerResponse` 和 `switch` 添加第三个成员。

注意，在将值与 `switch` 的各个分支进行匹配的过程中，日出和日落时间是如何从 `ServerResponse` 值中提取出来的。

使用 `struct` 创建结构体。结构体支持许多与类相同的行为，包括方法和构造器。结构体和类最重要的区别之一是：结构体在代码中传递时总是被拷贝，而类是按引用传递的。

```swift
struct Card {
    var rank: Rank
    var suit: Suit
    func simpleDescription() -> String {
        return "The \(rank.simpleDescription()) of \(suit.simpleDescription())"
    }
}
let threeOfSpades = Card(rank: .three, suit: .spades)
let threeOfSpadesDescription = threeOfSpades.simpleDescription()
```

> 练习：写一个函数，返回一个包含一副完整纸牌的数组，每种点数和花色组合各一张牌。

## 并发 {#Concurrency}

使用 `async` 标记异步运行的函数。

```swift
func fetchUserID(from server: String) async -> Int {
    if server == "primary" {
        return 97
    }
    return 501
}
```

在调用异步函数时，在前面写 `await` 来标记该调用。

```swift
func fetchUsername(from server: String) async -> String {
    let userID = await fetchUserID(from: server)
    if userID == 501 {
        return "John Appleseed"
    }
    return "Guest"
}
```

使用 `async let` 调用异步函数，让它与其他异步代码并行运行。使用它返回的值时，要写 `await`。

```swift
func connectUser(to server: String) async {
    async let userID = fetchUserID(from: server)
    async let username = fetchUsername(from: server)
    let greeting = await "Hello \(username), user ID \(userID)"
    print(greeting)
}
```

使用 `Task` 从同步代码中调用异步函数，而不必等待它们返回。

```swift
Task {
    await connectUser(to: "primary")
}
// Prints "Hello Guest, user ID 97".
```

使用任务组来组织并发代码。

```swift
let userIDs = await withTaskGroup(of: Int.self) { group in
    for server in ["primary", "secondary", "development"] {
        group.addTask {
            return await fetchUserID(from: server)
        }
    }

    var results: [Int] = []
    for await result in group {
        results.append(result)
    }
    return results
}
```

actor 与类类似，不同之处在于它们确保不同的异步函数可以安全地与同一个 actor 的实例同时交互。

```swift
actor ServerConnection {
    var server: String = "primary"
    private var activeUsers: [Int] = []
    func connect() async -> Int {
        let userID = await fetchUserID(from: server)
        // ... communicate with server ...
        activeUsers.append(userID)
        return userID
    }
}
```

当你调用 actor 上的方法或访问它的某个属性时，要用 `await` 标记这段代码，表示它可能需要等待已经在 actor 上运行的其他代码完成。

```swift
let server = ServerConnection()
let userID = await server.connect()
```

## 协议和扩展 {#Protocols-and-Extensions}

使用 `protocol` 声明协议。

```swift
protocol ExampleProtocol {
     var simpleDescription: String { get }
     mutating func adjust()
}
```

类、枚举和结构体都可以采纳协议。

```swift
class SimpleClass: ExampleProtocol {
     var simpleDescription: String = "A very simple class."
     var anotherProperty: Int = 69105
     func adjust() {
          simpleDescription += "  Now 100% adjusted."
     }
}
var a = SimpleClass()
a.adjust()
let aDescription = a.simpleDescription

struct SimpleStructure: ExampleProtocol {
     var simpleDescription: String = "A simple structure"
     mutating func adjust() {
          simpleDescription += " (adjusted)"
     }
}
var b = SimpleStructure()
b.adjust()
let bDescription = b.simpleDescription
```

> 练习：给 `ExampleProtocol` 添加另一个要求。你需要对 `SimpleClass` 和 `SimpleStructure` 做哪些修改，才能让它们仍然遵循该协议？

注意 `SimpleStructure` 的声明中使用了 `mutating` 关键字，用来标记修改结构体的方法。`SimpleClass` 的声明不需要把任何方法标记为 mutating，因为类上的方法总是可以修改该类。

使用 `extension` 给已有类型添加功能，例如新方法和计算属性。你可以用扩展给在其他地方声明的类型，甚至是从库或框架导入的类型，添加协议一致性。

```swift
extension Int: ExampleProtocol {
    var simpleDescription: String {
        return "The number \(self)"
    }
    mutating func adjust() {
        self += 42
    }
 }
print(7.simpleDescription)
// Prints "The number 7".
```

> 练习：为 `Double` 类型写一个扩展，添加一个 `absoluteValue` 属性。

你可以像使用任何其他具名类型一样使用协议名——例如，创建一组类型不同但都遵循同一个协议的对象。当你处理类型为装箱协议类型的值时，协议定义之外的方法不可用。

```swift
let protocolValue: any ExampleProtocol = a
print(protocolValue.simpleDescription)
// Prints "A very simple class.  Now 100% adjusted."
// print(protocolValue.anotherProperty)  // Uncomment to see the error
```

尽管变量 `protocolValue` 的运行时类型是 `SimpleClass`，编译器却把它当作 `ExampleProtocol` 类型处理。这意味着你不能意外访问该类在遵循协议之外还实现的方法或属性。

## 错误处理 {#Error-Handling}

使用任何遵循 `Error` 协议的类型来表示错误。

```swift
enum PrinterError: Error {
    case outOfPaper
    case noToner
    case onFire
}
```

使用 `throw` 抛出错误，使用 `throws` 标记可以抛出错误的函数。如果你在函数中抛出错误，函数会立即返回，调用该函数的代码会处理这个错误。

```swift
func send(job: Int, toPrinter printerName: String) throws -> String {
    if printerName == "Never Has Toner" {
        throw PrinterError.noToner
    }
    return "Job sent"
}
```

处理错误有几种方法。一种是使用 `do`-`catch`。在 `do` 块中，在可能抛出错误的代码前面写 `try` 来标记它。在 `catch` 块中，错误会自动命名为 `error`，除非你给它指定不同的名字。

```swift
do {
    let printerResponse = try send(job: 1040, toPrinter: "Bi Sheng")
    print(printerResponse)
} catch {
    print(error)
}
// Prints "Job sent".
```

> 练习：把打印机名字改成 `"Never Has Toner"`，让 `send(job:toPrinter:)` 函数抛出错误。

你可以提供多个 `catch` 块来处理特定的错误。在 `catch` 后面写模式，就像在 `switch` 中 `case` 后面写模式一样。

```swift
do {
    let printerResponse = try send(job: 1440, toPrinter: "Gutenberg")
    print(printerResponse)
} catch PrinterError.onFire {
    print("I'll just put this over here, with the rest of the fire.")
} catch let printerError as PrinterError {
    print("Printer error: \(printerError).")
} catch {
    print(error)
}
// Prints "Job sent".
```

> 练习：在 `do` 块中添加代码来抛出一个错误。你需要抛出什么样的错误，才能让该错误由第一个 `catch` 块处理？第二个和第三个块呢？

处理错误的另一种方法是使用 `try?` 把结果转换为可选值。如果函数抛出错误，具体的错误会被丢弃，结果为 `nil`。否则，结果是一个包含函数返回值的可选值。

```swift
let printerSuccess = try? send(job: 1884, toPrinter: "Mergenthaler")
let printerFailure = try? send(job: 1885, toPrinter: "Never Has Toner")
```

使用 `defer` 编写一个代码块，它在函数中所有其他代码之后、函数返回之前执行。无论函数是否抛出错误，这段代码都会执行。你可以用 `defer` 把设置代码和清理代码写在相邻的位置，即使它们需要在不同的时间执行。

```swift
var fridgeIsOpen = false
let fridgeContent = ["milk", "eggs", "leftovers"]

func fridgeContains(_ food: String) -> Bool {
    fridgeIsOpen = true
    defer {
        fridgeIsOpen = false
    }

    let result = fridgeContent.contains(food)
    return result
}
if fridgeContains("banana") {
    print("Found a banana")
}
print(fridgeIsOpen)
// Prints "false".
```

## 泛型 {#Generics}

在尖括号中写一个名字，创建泛型函数或泛型类型。

```swift
func makeArray<Item>(repeating item: Item, numberOfTimes: Int) -> [Item] {
    var result: [Item] = []
    for _ in 0..<numberOfTimes {
         result.append(item)
    }
    return result
}
makeArray(repeating: "knock", numberOfTimes: 4)
```

你可以创建函数和方法的泛型形式，也可以创建类、枚举和结构体的泛型形式。

```swift
// Reimplement the Swift standard library's optional type
enum OptionalValue<Wrapped> {
    case none
    case some(Wrapped)
}
var possibleInteger: OptionalValue<Int> = .none
possibleInteger = .some(100)
```

在函数体前面使用 `where` 指定一组要求——例如，要求类型实现某个协议、要求两个类型相同，或者要求某个类具有特定的父类。

```swift
func anyCommonElements<T: Sequence, U: Sequence>(_ lhs: T, _ rhs: U) -> Bool
    where T.Element: Equatable, T.Element == U.Element
{
    for lhsItem in lhs {
        for rhsItem in rhs {
            if lhsItem == rhsItem {
                return true
            }
        }
    }
   return false
}
anyCommonElements([1, 2, 3], [3])
```

> 练习：修改 `anyCommonElements(_:_:)` 函数，写一个返回任意两个序列共有元素组成的数组的函数。

写 `<T: Equatable>` 与写 `<T> ... where T: Equatable` 相同。
