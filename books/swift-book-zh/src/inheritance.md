# 继承 {#Inheritance}

通过子类化来添加或重写功能。

类可以从另一个类*继承*方法、属性和其他特征。当一个类从另一个类继承时，进行继承的类称为*子类*（subclass），被继承的类称为它的*父类*（superclass）。继承是 Swift 中让类区别于其他类型的一种基本行为。

Swift 中的类可以调用和访问属于其父类的方法、属性和下标，并且可以为这些方法、属性和下标提供自己的重写版本，以改进或修改它们的行为。Swift 会检查重写定义是否有匹配的父类定义，以此帮助确保你的重写是正确的。

类还可以为继承的属性添加属性观察器，以便在属性值发生变化时收到通知。无论属性最初定义为存储属性还是计算属性，都可以为它添加属性观察器。

## 定义基类 {#Defining-a-Base-Class}

不从另一个类继承的任何类都称为*基类*。

> 注意：Swift 中的类并不继承自某个通用的基类。你定义类时如果没有指定父类，这个类会自动成为基类，供你在其基础上继续构建。

下面的例子定义了一个名为 `Vehicle` 的基类。这个基类定义了一个名为 `currentSpeed` 的存储属性，默认值为 `0.0`（推断出的属性类型为 `Double`）。一个名为 `description` 的只读计算 `String` 属性会使用 `currentSpeed` 属性的值，来生成对车辆的描述。

`Vehicle` 基类还定义了一个名为 `makeNoise` 的方法。对于基类 `Vehicle` 的实例，这个方法实际上什么都不做，但之后会由 `Vehicle` 的子类进行定制：

```swift
class Vehicle {
    var currentSpeed = 0.0
    var description: String {
        return "traveling at \(currentSpeed) miles per hour"
    }
    func makeNoise() {
        // do nothing - an arbitrary vehicle doesn't necessarily make a noise
    }
}
```

你可以使用*构造器语法*（initializer syntax）创建 `Vehicle` 的新实例，写法是类型名后跟一对空括号：

```swift
let someVehicle = Vehicle()
```

创建新的 `Vehicle` 实例后，你可以访问它的 `description` 属性，打印出可读的车辆当前速度描述：

```swift
print("Vehicle: \(someVehicle.description)")
// Vehicle: traveling at 0.0 miles per hour
```

`Vehicle` 类定义了一辆普通车辆的通用特征，但本身用处不大。为了让它更有用，你需要对它进行改进，以便描述更具体的车辆类型。

## 子类化 {#Subclassing}

*子类化*（subclassing）是指在一个已有类的基础上创建新类的行为。子类从已有类继承特征，然后你可以对这些特征进行改进。你也可以为子类添加新的特征。

要表明某个子类有父类，请把子类名写在父类名之前，中间用冒号隔开：

```swift
class SomeSubclass: SomeSuperclass {
    // subclass definition goes here
}
```

下面的例子定义了一个名为 `Bicycle` 的子类，其父类为 `Vehicle`：

```swift
class Bicycle: Vehicle {
    var hasBasket = false
}
```

新的 `Bicycle` 类会自动获得 `Vehicle` 的所有特征，例如它的 `currentSpeed` 和 `description` 属性以及 `makeNoise()` 方法。

除了继承来的特征之外，`Bicycle` 类还定义了一个新的存储属性 `hasBasket`，默认值为 `false`（推断出的属性类型为 `Bool`）。

默认情况下，你创建的任何新 `Bicycle` 实例都没有篮子。你可以在创建某个 `Bicycle` 实例之后，把该实例的 `hasBasket` 属性设置为 `true`：

```swift
let bicycle = Bicycle()
bicycle.hasBasket = true
```

你还可以修改 `Bicycle` 实例继承来的 `currentSpeed` 属性，并查询该实例继承来的 `description` 属性：

```swift
bicycle.currentSpeed = 15.0
print("Bicycle: \(bicycle.description)")
// Bicycle: traveling at 15.0 miles per hour
```

子类本身还可以再被其他类子类化。下一个例子为 `Bicycle` 创建了一个子类，用来表示一种称为“tandem”的双座自行车：

```swift
class Tandem: Bicycle {
    var currentNumberOfPassengers = 0
}
```

`Tandem` 继承了 `Bicycle` 的所有属性和方法，而 `Bicycle` 又继承了 `Vehicle` 的所有属性和方法。`Tandem` 子类还添加了一个名为 `currentNumberOfPassengers` 的新存储属性，默认值为 `0`。

如果你创建 `Tandem` 的实例，就可以使用它的任何新属性和继承属性，并查询它从 `Vehicle` 继承的只读 `description` 属性：

```swift
let tandem = Tandem()
tandem.hasBasket = true
tandem.currentNumberOfPassengers = 2
tandem.currentSpeed = 22.0
print("Tandem: \(tandem.description)")
// Tandem: traveling at 22.0 miles per hour
```

## 重写 {#Overriding}

对于本应从父类继承的实例方法、类型方法、实例属性、类型属性或下标，子类可以提供自己的自定义实现。这称为*重写*（overriding）。

要重写本应继承的特征，请在重写定义前面加上 `override` 关键字。这样做可以明确表示你是有意提供重写，而不是碰巧提供了一个匹配的定义。意外重写可能导致意外行为，因此在编译代码时，任何没有 `override` 关键字的重写都会被诊断为错误。

`override` 关键字还会让 Swift 编译器检查：你进行重写的类的父类（或它的某个祖先类）中，是否有一个声明与你提供的重写定义相匹配。这项检查确保你的重写定义是正确的。

### 访问父类的方法、属性和下标 {#Accessing-Superclass-Methods-Properties-and-Subscripts}

当你为子类重写方法、属性或下标时，有时把父类已有的实现作为重写的一部分会很有用。例如，你可以改进该已有实现的行为，或者把修改后的值存储到继承来的已有变量中。

在适合这样做时，你可以通过 `super` 前缀来访问方法、属性或下标的父类版本：

- 名为 `someMethod()` 的重写方法，可以在重写方法的实现中调用 `super.someMethod()`，从而调用 `someMethod()` 的父类版本。
- 名为 `someProperty` 的重写属性，可以在重写的 getter 或 setter 实现中通过 `super.someProperty` 访问 `someProperty` 的父类版本。
- 针对 `someIndex` 的重写下标，可以在重写下标的实现中通过 `super[someIndex]` 访问同一个下标的父类版本。

### 重写方法 {#Overriding-Methods}

你可以重写继承来的实例方法或类型方法，在子类中为该方法提供定制或替代的实现。

下面的例子定义了 `Vehicle` 的一个名为 `Train` 的新子类，它重写了 `Train` 从 `Vehicle` 继承的 `makeNoise()` 方法：

```swift
class Train: Vehicle {
    override func makeNoise() {
        print("Choo Choo")
    }
}
```

如果你创建 `Train` 的新实例并调用它的 `makeNoise()` 方法，可以看到调用的是 `Train` 子类版本的方法：

```swift
let train = Train()
train.makeNoise()
// Prints "Choo Choo".
```

### 重写属性 {#Overriding-Properties}

你可以重写继承来的实例属性或类型属性，为该属性提供自己的自定义 getter 和 setter；也可以添加属性观察器，让重写后的属性能够观察底层属性值何时发生变化。

#### 重写属性的 getter 和 setter {#Overriding-Property-Getters-and-Setters}

你可以提供自定义 getter（以及 setter，如果合适的话）来重写*任何*继承属性，无论该继承属性在源码中实现为存储属性还是计算属性。子类并不知道继承属性是存储属性还是计算属性——它只知道该继承属性有某个名称和类型。你必须始终同时写明被重写属性的名称和类型，以便编译器检查你的重写是否与父类中同名同类型的属性相匹配。

在子类的属性重写中同时提供 getter 和 setter，就可以把继承来的只读属性呈现为读写属性。但是，你不能把继承来的读写属性呈现为只读属性。

> 注意：如果你在属性重写中提供了 setter，就必须同时为该重写提供 getter。如果你不想在重写的 getter 中修改继承属性的值，可以简单地在 getter 中返回 `super.someProperty`，把继承的值原样传出，其中 `someProperty` 是你正在重写的属性的名称。

下面的例子定义了一个名为 `Car` 的新类，它是 `Vehicle` 的子类。`Car` 类引入了一个名为 `gear` 的新存储属性，默认值为整数 `1`。`Car` 类还重写了它从 `Vehicle` 继承的 `description` 属性，以提供包含当前挡位的自定义描述：

```swift
class Car: Vehicle {
    var gear = 1
    override var description: String {
        return super.description + " in gear \(gear)"
    }
}
```

`description` 属性的重写首先调用 `super.description`，它返回 `Vehicle` 类的 `description` 属性。然后，`Car` 类版本的 `description` 会在这段描述的末尾添加一些额外的文本，以提供当前挡位的信息。

如果你创建 `Car` 类的实例并设置它的 `gear` 和 `currentSpeed` 属性，可以看到它的 `description` 属性返回的是 `Car` 类中定义的定制描述：

```swift
let car = Car()
car.currentSpeed = 25.0
car.gear = 3
print("Car: \(car.description)")
// Car: traveling at 25.0 miles per hour in gear 3
```

#### 重写属性观察器 {#Overriding-Property-Observers}

你可以通过重写属性来为继承的属性添加属性观察器。这样一来，无论该属性最初是如何实现的，当继承属性的值发生变化时，你都能收到通知。关于属性观察器的更多信息，请参阅 [属性观察器](properties.md#Property-Observers)。

> 注意：你不能为继承来的常量存储属性或继承来的只读计算属性添加属性观察器。这些属性的值无法设置，因此不适合在重写中提供 `willSet` 或 `didSet` 实现。
>
> 另请注意，你不能为同一个属性同时提供重写的 setter 和重写的属性观察器。如果你想观察属性值的变化，而且已经为该属性提供了自定义 setter，那么只需在自定义 setter 中观察值的变化即可。

下面的例子定义了一个名为 `AutomaticCar` 的新类，它是 `Car` 的子类。`AutomaticCar` 类表示一辆带自动变速箱的汽车，它会根据当前速度自动选择合适的挡位：

```swift
class AutomaticCar: Car {
    override var currentSpeed: Double {
        didSet {
            gear = Int(currentSpeed / 10.0) + 1
        }
    }
}
```

每当你设置 `AutomaticCar` 实例的 `currentSpeed` 属性时，该属性的 `didSet` 观察器就会把实例的 `gear` 属性设置为适合新速度的挡位。具体来说，属性观察器选择的挡位等于新的 `currentSpeed` 值除以 `10`、向下取整到最接近的整数，再加 `1`。速度为 `35.0` 时，挡位为 `4`：

```swift
let automatic = AutomaticCar()
automatic.currentSpeed = 35.0
print("AutomaticCar: \(automatic.description)")
// AutomaticCar: traveling at 35.0 miles per hour in gear 4
```

## 阻止重写 {#Preventing-Overrides}

你可以通过把方法、属性或下标标记为 *final*，来阻止它被重写。做法是在方法、属性或下标的引入关键字（introducer keyword）前面写 `final` 修饰符（例如 `final var`、`final func`、`final class func` 和 `final subscript`）。

任何在子类中重写 final 方法、属性或下标的尝试，都会报告为编译时错误。你在扩展中为类添加的方法、属性或下标，也可以在扩展的定义中标记为 final。更多信息请参阅 [扩展](extensions.md)。

你可以在类定义的 `class` 关键字前面写 `final` 修饰符（`final class`），从而把整个类标记为 final。任何试图把 final 类子类化的尝试，都会报告为编译时错误。
