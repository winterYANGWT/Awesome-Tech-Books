# 并发 {#Concurrency}

执行异步操作。

Swift 内建支持以结构化的方式编写异步和并行代码。*异步代码*（asynchronous code）可以被挂起，稍后再恢复，尽管程序在任一时刻只执行一部分代码。挂起和恢复程序中的代码，可以让程序在继续处理耗时较长的操作（例如从网络获取数据或解析文件）的同时，在更新 UI 这样的短期操作上继续取得进展。*并行代码*（parallel code）意味着多部分代码同时运行——例如，一台配有四核处理器的电脑可以同时运行四部分代码，每个核心执行其中一个任务。使用并行和异步代码的程序会同时执行多个操作，并挂起那些正在等待外部系统的操作。本章其余部分用*并发*（concurrency）这个术语来指代异步和并行代码的这种常见组合。

并行或异步代码带来的额外调度灵活性，也伴随着复杂度增加的代价。编写并发代码时，你无法预先知道哪些代码会同时运行，而且你可能并不总能知道代码运行的顺序。并发代码中的一个常见问题是多部分代码试图访问同一份共享可变状态——这被称为*数据竞争*（data race）。当你使用语言层面的并发支持时，Swift 会检测并防止数据竞争，大多数数据竞争会产生编译时错误。有些数据竞争直到代码运行时才能被检测到；这些数据竞争会终止代码执行。你使用 actor 和隔离来防止数据竞争，本章会对此进行介绍。

> 注意：如果你以前写过并发代码，可能已经习惯使用线程。Swift 中的并发模型构建在线程之上，但你不需要直接与线程交互。Swift 中的异步函数可以让出它正在运行的线程，从而让另一个异步函数在该线程上运行，而第一个函数则处于阻塞状态。当异步函数恢复时，Swift 不保证该函数会在哪个线程上运行。

虽然不使用 Swift 的语言支持也可以编写并发代码，但这样的代码往往更难阅读。例如，下面的代码下载一组照片名称，下载该列表中的第一张照片，并向用户显示这张照片：

```swift
listPhotos(inGallery: "Summer Vacation") { photoNames in
    let sortedNames = photoNames.sorted()
    let name = sortedNames[0]
    downloadPhoto(named: name) { photo in
        show(photo)
    }
}
```

即使在这个简单的例子中，由于代码必须写成一系列完成处理程序，你最终会写出嵌套的闭包。在这种风格下，嵌套很深的复杂代码很快就会变得难以驾驭。

## 定义和调用异步函数 {#Defining-and-Calling-Asynchronous-Functions}

*异步函数*（asynchronous function）或*异步方法*（asynchronous method）是一种特殊的函数或方法，它可以在执行到一半时被挂起。这与普通的同步函数和方法形成对比：后者要么运行到结束，要么抛出一个错误，要么永不返回。异步函数或方法仍然会做这三件事之一，但它在等待某些东西时也可以中途暂停。在异步函数或方法的函数体中，你要标出每一个执行可能被挂起的位置。

要表示一个函数或方法是异步的，可以在其声明的参数之后写上 `async` 关键字，类似于用 `throws` 标记抛出函数。如果函数或方法返回值，就把 `async` 写在返回箭头（`->`）之前。例如，下面是你可能用来获取图库中照片名称的写法：

```swift
func listPhotos(inGallery name: String) async -> [String] {
    let result = // ... some asynchronous networking code ...
    return result
}
```

对于既是异步又会抛出的函数或方法，把 `async` 写在 `throws` 之前。

调用异步方法时，执行会挂起，直到该方法返回。你要在调用前写上 `await`，以标记可能的挂起点。这类似于调用抛出函数时写 `try`，用来标记出错时程序流程可能发生的变化。在异步方法内部，执行流程*只有*在调用另一个异步方法时才可能被挂起——挂起从来不是隐式的或抢占式的——这意味着每个可能的挂起点都用 `await` 标记了。在代码中标记出所有可能的挂起点，有助于让并发代码更易于阅读和理解。

例如，下面的代码获取图库中所有图片的名称，然后显示第一张图片：

```swift
let photoNames = await listPhotos(inGallery: "Summer Vacation")
let sortedNames = photoNames.sorted()
let name = sortedNames[0]
let photo = await downloadPhoto(named: name)
show(photo)
```

由于 `listPhotos(inGallery:)` 和 `downloadPhoto(named:)` 函数都需要发起网络请求，它们可能需要相对较长的时间才能完成。把 `async` 写在返回箭头之前让它们都成为异步函数，就能让 App 的其他代码在这段代码等待图片准备就绪时继续运行。

为了理解上面示例的并发性质，下面是一种可能的执行顺序：

1. 代码从第一行开始运行，一直运行到第一个 `await`。它调用 `listPhotos(inGallery:)` 函数，并在等待该函数返回时挂起执行。

2. 当这段代码的执行被挂起时，同一程序中的其他一些并发代码会运行。例如，可能有一个长时间运行的后台任务在继续更新新照片图库的列表。那段代码也会一直运行到下一个由 `await` 标记的挂起点，或者运行到完成。

3. `listPhotos(inGallery:)` 返回后，这段代码从该点继续执行。它把返回的值赋给 `photoNames`。

4. 定义 `sortedNames` 和 `name` 的那几行是普通的同步代码。由于这些行上没有任何地方标记了 `await`，所以不存在任何可能的挂起点。

5. 下一个 `await` 标记了对 `downloadPhoto(named:)` 函数的调用。这段代码再次暂停执行，直到该函数返回，从而给其他并发代码运行的机会。

6. `downloadPhoto(named:)` 返回后，它的返回值被赋给 `photo`，然后在调用 `show(_:)` 时作为实参传入。

代码中用 `await` 标记的可能挂起点，表示当前这段代码可能在等待异步函数或方法返回时暂停执行。这也称为*让出线程*（yielding the thread），因为在幕后，Swift 会挂起你的代码在当前线程上的执行，转而改在该线程上运行其他代码。由于带有 `await` 的代码需要能够挂起执行，所以程序中只有某些位置可以调用异步函数或方法：

- 异步函数、方法或属性主体中的代码。

- 标记了 `@main` 的结构体、类或枚举的静态 `main()` 方法中的代码。

- 非结构化子任务中的代码，如下面的 [非结构化并发](concurrency.md#Unstructured-Concurrency) 所示。

在编写简单的代码来学习并发如何工作时，[`Task.sleep(for:tolerance:clock:)`][] 方法很有用。该方法会将当前任务挂起至少给定的时长。下面是 `listPhotos(inGallery:)` 函数的一个版本，它使用 `sleep(for:tolerance:clock:)` 来模拟等待网络操作：

[`Task.sleep(for:tolerance:clock:)`]: https://developer.apple.com/documentation/swift/task/sleep(for:tolerance:clock:)

```swift
func listPhotos(inGallery name: String) async throws -> [String] {
    try await Task.sleep(for: .seconds(2))
    return ["IMG001", "IMG99", "IMG0404"]
}
```

上面代码中的 `listPhotos(inGallery:)` 版本既是异步的，又可能抛出错误，因为对 `Task.sleep(until:tolerance:clock:)` 的调用可能抛出错误。调用这个版本的 `listPhotos(inGallery:)` 时，你要同时写 `try` 和 `await`：

```swift
let photos = try await listPhotos(inGallery: "A Rainy Weekend")
```

异步函数与抛出函数有一些相似之处：定义异步函数或抛出函数时，用 `async` 或 `throws` 标记它；调用该函数时，用 `await` 或 `try` 标记调用。异步函数可以调用另一个异步函数，就像抛出函数可以调用另一个抛出函数一样。

然而，有一个非常重要的区别。你可以把抛出错误的代码包在 `do`-`catch` 块中处理错误，也可以使用 `Result` 把错误存起来，交给其他地方的代码处理。这些方式让你可以从非抛出函数中调用抛出函数。例如：

```swift
func availableRainyWeekendPhotos() -> Result<[String], Error> {
    return Result {
        try listDownloadedPhotos(inGallery: "A Rainy Weekend")
    }
}
```

相比之下，没有安全的方式把异步代码包装起来，以便你可以从同步代码中调用它并等待结果。Swift 标准库有意省略了这项不安全的特性——尝试自己实现它可能会导致隐蔽的竞争、线程问题和死锁等。向现有项目添加并发代码时，要自顶向下进行。具体来说，先转换最上层的代码以使用并发，然后转换它调用的函数和方法，沿着项目的架构一层一层地进行。没有办法采用自底向上的方式，因为同步代码永远不能调用异步代码。

## 异步序列 {#Asynchronous-Sequences}

上一节中的 `listPhotos(inGallery:)` 函数在数组的所有元素都准备好之后，一次性异步返回整个数组。另一种方法是使用*异步序列*（asynchronous sequence），一次等待集合中的一个元素。下面是遍历异步序列的写法：

```swift
import Foundation

let handle = FileHandle.standardInput
for try await line in handle.bytes.lines {
    print(line)
}
```

上面的示例没有使用普通的 `for`-`in` 循环，而是在 `for` 后面写上 `await`。就像调用异步函数或方法时一样，写 `await` 表示一个可能的挂起点。`for`-`await`-`in` 循环可能在每次迭代开始时挂起执行，等待下一个元素可用。

正如你可以通过添加对 [`Sequence`][] 协议的遵循，在 `for`-`in` 循环中使用自己的类型一样，你也可以通过添加对 [`AsyncSequence`][] 协议的遵循，在 `for`-`await`-`in` 循环中使用自己的类型。

[`Sequence`]: https://developer.apple.com/documentation/swift/sequence [`AsyncSequence`]: https://developer.apple.com/documentation/swift/asyncsequence

## 并行调用异步函数 {#Calling-Asynchronous-Functions-in-Parallel}

用 `await` 调用异步函数时，一次只运行一部分代码。在异步代码运行期间，调用方会等待该代码完成，然后才继续执行下一行代码。例如，要从图库中获取前三张照片，可以像下面这样对 `downloadPhoto(named:)` 函数进行三次 await 调用：

```swift
let firstPhoto = await downloadPhoto(named: photoNames[0])
let secondPhoto = await downloadPhoto(named: photoNames[1])
let thirdPhoto = await downloadPhoto(named: photoNames[2])

let photos = [firstPhoto, secondPhoto, thirdPhoto]
show(photos)
```

这种方法有一个重要的缺点：尽管下载是异步的，并且可以在下载进行的同时开展其他工作，但对 `downloadPhoto(named:)` 的调用一次只运行一个。每张照片完全下载完后，下一张才开始下载。然而，这些操作没有必要彼此等待——每张照片都可以独立下载，甚至可以同时下载。

要调用异步函数并让它与周围的代码并行运行，可以在定义常量时把 `async` 写在 `let` 前面，然后在每次使用该常量时写 `await`。

```swift
async let firstPhoto = downloadPhoto(named: photoNames[0])
async let secondPhoto = downloadPhoto(named: photoNames[1])
async let thirdPhoto = downloadPhoto(named: photoNames[2])

let photos = await [firstPhoto, secondPhoto, thirdPhoto]
show(photos)
```

在这个例子中，对 `downloadPhoto(named:)` 的三个调用都在不等待前一个完成的情况下开始。如果有足够的系统资源可用，它们可以同时运行。这些函数调用都没有用 `await` 标记，因为代码不会挂起以等待函数的结果。相反，执行会一直继续到定义 `photos` 的那一行——此时程序需要这些异步调用的结果，所以你写 `await` 来暂停执行，直到三张照片都下载完成。

你可以这样理解这两种方法的区别：

- 当后面几行的代码依赖该函数的结果时，用 `await` 调用异步函数。这会创建按顺序执行的工作。
- 当你到代码后面才需要结果时，用 `async`-`let` 调用异步函数。这会创建可以并行执行的工作。
- `await` 和 `async`-`let` 都允许其他代码在它们挂起时运行。
- 在这两种情况下，你都用 `await` 标记可能的挂起点，表示执行会在需要时暂停，直到异步函数返回。

你也可以在同一份代码中混用这两种方法。

## 任务和任务组 {#Tasks-and-Task-Groups}

*任务*（task）是一个工作单元，可以作为程序的一部分异步运行。所有异步代码都作为某个任务的一部分运行。一个任务本身一次只做一件事，但当你创建多个任务时，Swift 可以调度它们同时运行。

上一节介绍的 `async`-`let` 语法会隐式创建一个子任务——当你已经知道程序需要运行哪些任务时，这种语法很好用。你也可以创建一个任务组（[`TaskGroup`][] 的实例），并向该组中显式添加子任务，这样可以更好地控制优先级和取消，并让你创建数量动态变化的任务。

[`TaskGroup`]: https://developer.apple.com/documentation/swift/taskgroup

任务按层级排列。给定任务组中的每个任务都有相同的父任务，而每个任务都可以有子任务。由于任务与任务组之间存在这种显式关系，这种方法被称为*结构化并发*（structured concurrency）。任务之间显式的父子关系有几个优点：

- 在父任务中，你不会忘记等待其子任务完成。

- 在给子任务设置更高优先级时，父任务的优先级会自动提升。

- 当父任务被取消时，它的每个子任务也会自动被取消。

- 任务局部值会高效且自动地传播到子任务。

下面是下载照片的另一个代码版本，它可以处理任意数量的照片：

```swift
await withTaskGroup(of: Data.self) { group in
    let photoNames = await listPhotos(inGallery: "Summer Vacation")
    for name in photoNames {
        group.addTask {
            return await downloadPhoto(named: name)
        }
    }

    for await photo in group {
        show(photo)
    }
}
```

上面的代码创建了一个新的任务组，然后创建子任务来下载图库中的每张照片。Swift 会在条件允许的情况下尽可能多地并发运行这些任务。一旦某个子任务完成照片下载，就会显示那张照片。子任务完成的顺序没有保证，因此这个图库中的照片可能以任意顺序显示。

> 注意：如果下载照片的代码可能抛出错误，你应该改为调用 `withThrowingTaskGroup(of:returning:body:)`。

在上面的代码清单中，每张照片下载后就会被显示，所以任务组不返回任何结果。对于要返回结果的任务组，你要在传给 `withTaskGroup(of:returning:body:)` 的闭包中添加累积结果的代码。

```swift
let photos = await withTaskGroup(of: Data.self) { group in
    let photoNames = await listPhotos(inGallery: "Summer Vacation")
    for name in photoNames {
        group.addTask {
            return await downloadPhoto(named: name)
        }
    }

    var results: [Data] = []
    for await photo in group {
        results.append(photo)
    }

    return results
}
```

和上一个例子一样，这个例子也为每张照片创建一个子任务来下载它。与上一个例子不同的是，`for`-`await`-`in` 循环会等待下一个子任务完成，把该任务的结果追加到结果数组中，然后继续等待，直到所有子任务都完成。最后，任务组把已下载照片的数组作为整体结果返回。

### 任务取消 {#Task-Cancellation}

Swift 并发使用协作式取消模型。每个任务在执行过程中的适当位置检查自己是否已被取消，并相应地响应取消。具体取决于任务正在做的工作，响应取消通常意味着执行以下操作之一：

- 抛出像 `CancellationError` 这样的错误
- 返回 `nil` 或空集合
- 返回部分完成的工作

如果图片很大或网络很慢，下载图片可能需要很长时间。为了让用户在不等待所有任务完成的情况下停止这项工作，任务需要检查取消情况，并在被取消时停止运行。任务有两种方式做到这一点：调用 [`Task.checkCancellation()`][] 类型方法，或读取 [`Task.isCancelled`][`Task.isCancelled` type] 类型属性。如果任务被取消，调用 `checkCancellation()` 会抛出错误；会抛出的任务可以把该错误传播到任务之外，从而停止该任务的所有工作。这种方式的优点是易于实现和理解。如果需要更大的灵活性，可以使用 `isCancelled` 属性，它让你可以在停止任务的过程中执行清理工作，例如关闭网络连接和删除临时文件。

[`Task.checkCancellation()`]: https://developer.apple.com/documentation/swift/task/checkcancellation() [`Task.isCancelled` type]: https://developer.apple.com/documentation/swift/task/iscancelled-swift.type.property

```swift
let photos = await withTaskGroup { group in
    let photoNames = await listPhotos(inGallery: "Summer Vacation")
    for name in photoNames {
        let added = group.addTaskUnlessCancelled {
            Task.isCancelled ? nil : await downloadPhoto(named: name)
        }
        guard added else { break }
    }

    var results: [Data] = []
    for await photo in group {
        if let photo { results.append(photo) }
    }
    return results
}
```

上面的代码与上一版本相比做了几处改动：

- 每个任务都使用 [`TaskGroup.addTaskUnlessCancelled(priority:operation:)`][] 方法添加，以避免在取消之后启动新工作。

- 每次调用 `addTaskUnlessCancelled(priority:operation:)` 之后，代码都会确认新的子任务已被添加。如果任务组被取消，`added` 的值就是 `false`——在这种情况下，代码会停止尝试下载更多照片。

- 每个任务在开始下载照片之前都会检查取消情况。如果任务已被取消，就返回 `nil`。

- 最后，任务组在收集结果时会跳过 `nil` 值。通过返回 `nil` 来处理取消，意味着任务组可以返回部分结果——取消时已经下载好的照片——而不是丢弃这些已完成的工作。

[`TaskGroup.addTaskUnlessCancelled(priority:operation:)`]: https://developer.apple.com/documentation/swift/taskgroup/addtaskunlesscancelled(priority:operation:)

> 注意：要从任务外部检查任务是否已被取消，请使用 [`Task.isCancelled`][`Task.isCancelled` instance] 实例属性，而不是类型属性。

[`Task.isCancelled` instance]: https://developer.apple.com/documentation/swift/task/iscancelled-swift.property

对于需要立即收到取消通知的工作，请使用 [`withTaskCancellationHandler(operation:onCancel:isolation:)`][] 函数。例如：

[`withTaskCancellationHandler(operation:onCancel:isolation:)`]: https://developer.apple.com/documentation/swift/withtaskcancellationhandler(operation:oncancel:isolation:)

```swift
let task = Task {
    await withTaskCancellationHandler {
        // ...
    } onCancel: {
        print("Canceled!")
    }
}

// ... some time later...
task.cancel()  // Prints "Canceled!"
```

使用取消处理程序时，任务取消仍然是协作式的：任务要么运行到完成，要么检查取消情况并提前停止。由于取消处理程序启动时任务仍在运行，要避免在任务与其取消处理程序之间共享状态，否则可能会产生竞争条件。

### 非结构化并发 {#Unstructured-Concurrency}

除了前几节介绍的结构化并发方法之外，Swift 还支持非结构化并发。与任务组中的任务不同，*非结构化任务*（unstructured task）没有父任务。你可以完全灵活地按照程序需要的方式管理非结构化任务，但你也必须完全对它们的正确性负责。

要创建一个与周围代码类似方式运行的非结构化任务，请调用 [`Task.init(name:priority:operation:)`][] 构造器。新任务默认与当前任务具有相同的 actor 隔离、优先级和任务局部状态。要创建一个更独立于周围代码的非结构化任务（更具体地称为*分离任务*，即 detached task），请调用 [`Task.detached(name:priority:operation:)`][] 静态方法。新任务默认在没有任何 actor 隔离的情况下运行，并且不继承当前任务的优先级或任务局部状态。这两种操作都会返回一个你可以与之交互的任务——例如，等待它的结果或取消它。

```swift
let newPhoto = // ... some photo data ...
let handle = Task {
    return await add(newPhoto, toGalleryNamed: "Spring Adventures")
}
let result = await handle.value
```

有关管理分离任务的更多信息，参见 [`Task`](https://developer.apple.com/documentation/swift/task)。

[`Task.init(name:priority:operation:)`]: https://developer.apple.com/documentation/swift/task/init(name:priority:operation:)-43wmk [`Task.detached(name:priority:operation:)`]: https://developer.apple.com/documentation/swift/task/detached(name:priority:operation:)-795w1

## 隔离 {#Isolation}

前几节讨论了拆分并发工作的各种方法。这些工作可能涉及修改共享数据，例如 App 的 UI。如果代码的不同部分可能同时修改同一份数据，就有可能产生数据竞争。Swift 会保护你的代码免受数据竞争的影响：每当你读取或修改一份数据时，Swift 都会确保没有其他代码同时修改它。这种保证称为*数据隔离*（data isolation）。有三种主要方式可以隔离数据：

1. 不可变数据始终是隔离的。由于你无法修改常量，所以不存在其他代码在你读取某个常量的同时修改它的风险。

2. 只被当前任务引用的数据始终是隔离的。局部变量可以安全地读写，因为任务之外的代码没有指向那块内存的引用，所以没有其他代码可以修改该数据。此外，如果你在闭包中捕获该变量，Swift 会确保该闭包不会被并发使用。

3. 受 actor 保护的数据，如果访问该数据的代码也隔离到该 actor，那么这些数据就是隔离的。如果当前函数隔离到某个 actor，那么读写受该 actor 保护的数据就是安全的，因为任何其他隔离到同一个 actor 的代码都必须等待轮到自己才能运行。

## 主 actor {#The-Main-Actor}

actor 是一种对象，它通过强制代码轮流访问可变数据来保护对可变数据的访问。在许多程序中，最重要的 actor 是*主 actor*（main actor）。在 App 中，主 actor 保护所有用于显示 UI 的数据。主 actor 轮流渲染 UI、处理 UI 事件，以及运行你编写的需要查询或更新 UI 的代码。

在你开始使用并发之前，所有代码都在主 actor 上运行。随着你识别出耗时较长或资源密集型的代码，你可以把这项工作移出主 actor，同时仍然保持安全和正确。

> 注意：主 actor 与主线程密切相关，但它们不是一回事。主 actor 有私有的可变状态，而主线程会序列化对该状态的访问。当你在主 actor 上运行代码时，Swift 会在主线程上执行该代码。由于这种联系，你可能会看到这两个术语被互换使用。你的代码与主 actor 交互；主线程是更底层的实现细节。

有几种方式可以在主 actor 上运行工作。要确保函数始终在主 actor 上运行，请用 `@MainActor` 特性标记它：

```swift
@MainActor
func show(_: Data) {
    // ... UI code to display the photo ...
}
```

在上面的代码中，`show(_:)` 函数上的 `@MainActor` 特性要求该函数只在主 actor 上运行。在其他运行于主 actor 上的代码中，你可以把 `show(_:)` 当作同步函数来调用。然而，要从不在主 actor 上运行的代码中调用 `show(_:)`，你必须加上 `await`，并把它当作异步函数来调用，因为切换到主 actor 会引入一个可能的挂起点。例如：

```swift
func downloadAndShowPhoto(named name: String) async {
    let photo = await downloadPhoto(named: name)
    await show(photo)
}
```

在上面的代码中，`downloadPhoto(named:)` 和 `show(_:)` 这两个函数在调用时都可能挂起。这段代码还展示了一种常见模式：在后台执行耗时较长且 CPU 密集型的工作，然后切换到主 actor 来更新 UI。由于 `downloadAndShowPhoto(named:)` 函数不在主 actor 上，`downloadPhoto(named:)` 中的工作也不在主 actor 上运行。只有 `show(_:)` 中更新 UI 的工作在主 actor 上运行，因为该函数用 `@MainActor` 特性标记了。

要确保闭包在主 actor 上运行，请在捕获列表之前、`in` 之前，也就是闭包的开头写 `@MainActor`。

```swift
let photo = await downloadPhoto(named: "Trees at Sunrise")
Task { @MainActor in
    show(photo)
}
```

上面的代码与上一个代码清单中的 `downloadAndShowPhoto(named:)` 类似，但这个例子中的代码不会等待 UI 更新。你也可以在结构体、类或枚举上写 `@MainActor`，以确保它的所有方法以及对其属性的所有访问都在主 actor 上运行：

```swift
@MainActor
struct PhotoGallery {
    var photoNames: [String]
    func drawUI() { /* ... other UI code ... */ }
}
```

上面代码中的 `PhotoGallery` 结构体会在屏幕上绘制照片，它使用 `photoNames` 属性中的名称来确定要显示哪些照片。由于 `photoNames` 会影响 UI，修改它的代码需要在主 actor 上运行，以序列化该访问。

当你基于某个框架构建时，该框架的协议和基类通常已经标记了 `@MainActor`，因此在这种情况下，你通常不需要在自己的类型上写 `@MainActor`。下面是一个简化示例：

```swift
@MainActor
protocol View { /* ... */ }

// Implicitly @MainActor
struct PhotoGalleryView: View { /* ... */ }
```

在上面的代码中，像 SwiftUI 这样的框架定义了 `View` 协议。通过在协议声明上写 `@MainActor`，遵循该协议的类型（如 `PhotoGalleryView`）也会被隐式标记为 `@MainActor`。如果 `View` 是基类而 `PhotoGalleryView` 是子类，你也会看到同样的行为——子类会被隐式标记为 `@MainActor`。

在上面的示例中，`PhotoGallery` 把整个结构体都放在主 actor 的保护之下。如果需要更细粒度的控制，你可以只给那些需要在主线程上访问或运行的属性或方法写 `@MainActor`：

```swift
struct PhotoGallery {
    @MainActor var photoNames: [String]
    var hasCachedPhotos = false

    @MainActor func drawUI() { /* ... UI code ... */ }
    func cachePhotos() { /* ... networking code ... */ }
}
```

在上面这个版本的 `PhotoGallery` 中，`drawUI()` 方法把图库中的图片绘制到屏幕上，因此它需要隔离到主 actor。`photoNames` 属性不直接创建 UI，但它确实存储了 `drawUI()` 函数用来绘制 UI 的状态，所以这个属性也必须在主 actor 上访问。相比之下，对 `hasCachedPhotos` 属性的修改不会与 UI 交互，`cachePhotos()` 方法也不访问任何需要在主 actor 上运行的状态。因此这些都没有用 `@MainActor` 标记。

与前面的示例一样，如果你使用某个框架来构建 UI，该框架的属性包装器可能已经把 UI 状态属性标记为 `@MainActor`。在定义属性包装器时，如果它的 `wrappedValue` 属性被标记为 `@MainActor`，那么你应用该属性包装器的任何属性也会被隐式标记为 `@MainActor`。

## actor {#Actors}

Swift 为你提供了主 actor——你也可以定义自己的 actor。actor 让你可以在并发代码之间安全地共享信息。

和类一样，actor 是引用类型，因此 [类是引用类型](classes-and-structures.md#Classes-Are-Reference-Types) 中关于值类型和引用类型的比较同样适用于 actor 和类。与类不同的是，actor 一次只允许一个任务访问其可变状态，这使得多个任务中的代码可以安全地与同一个 actor 实例交互。例如，下面是一个记录温度的 actor：

```swift
actor TemperatureLogger {
    let label: String
    var measurements: [Int]
    private(set) var max: Int

    init(label: String, measurement: Int) {
        self.label = label
        self.measurements = [measurement]
        self.max = measurement
    }
}
```

你用 `actor` 关键字引入一个 actor，后面跟一对花括号中的定义。`TemperatureLogger` actor 有一些 actor 外部其他代码可以访问的属性，并把 `max` 属性限制为只有 actor 内部的代码才能更新最大值。

创建 actor 实例的方式与创建结构体和类实例的构造器语法相同。当你访问 actor 的属性或方法时，要用 `await` 标记可能的挂起点。例如：

```swift
let logger = TemperatureLogger(label: "Outdoors", measurement: 25)
print(await logger.max)
// Prints "25".
```

在这个例子中，访问 `logger.max` 是一个可能的挂起点。由于 actor 一次只允许一个任务访问其可变状态，如果来自另一个任务的代码已经在与这个 logger 交互，那么这段代码就会挂起，等待能够访问该属性。

相比之下，作为 actor 一部分的代码在访问 actor 的属性时不写 `await`。例如，下面是一个用新温度更新 `TemperatureLogger` 的方法：

```swift
extension TemperatureLogger {
    func update(with measurement: Int) {
        measurements.append(measurement)
        if measurement > max {
            max = measurement
        }
    }
}
```

`update(with:)` 方法已经在 actor 上运行，所以它在访问 `max` 这类属性时不用 `await` 标记。这个方法还展示了 actor 为什么一次只允许一个任务与其可变状态交互的原因之一：对 actor 状态的某些更新会暂时破坏不变量。`TemperatureLogger` actor 会跟踪温度列表和最高温度，并在记录新测量值时更新最高温度。在更新过程中，追加新测量值之后、更新 `max` 之前，温度记录器处于暂时的不一致状态。防止多个任务同时与同一个实例交互，可以避免下面这类问题：

1. 你的代码调用 `update(with:)` 方法。它先更新 `measurements` 数组。
2. 在你的代码来得及更新 `max` 之前，其他地方的代码读取了最大值和温度数组。
3. 你的代码通过修改 `max` 完成更新。

在这种情况下，其他地方运行的代码会读到错误的信息，因为它对 actor 的访问被穿插在 `update(with:)` 调用中间，而此时数据暂时无效。使用 Swift actor 时可以防止这个问题，因为它们一次只允许对其状态进行一个操作，而且这些代码只能在 `await` 标记挂起点的地方被中断。由于 `update(with:)` 不包含任何挂起点，所以没有其他代码可以在更新过程中访问数据。

如果 actor 外部的代码试图直接访问这些属性，就像访问结构体或类的属性那样，你会得到一个编译时错误。例如：

```swift
print(logger.max)  // Error
```

不写 `await` 就访问 `logger.max` 会失败，因为 actor 的属性是该 actor 隔离的本地状态的一部分。访问这个属性的代码需要作为 actor 的一部分运行，而这是一个异步操作，需要写 `await`。Swift 保证只有运行在 actor 上的代码才能访问该 actor 的本地状态。这种保证称为*actor 隔离*（actor isolation）。

Swift 并发模型的以下几个方面协同工作，使共享可变状态更容易推理：

- 可能挂起点之间的代码按顺序运行，不会被其他并发代码中断。不过，多部分并发代码可以同时运行，所以其他代码可能也在同时运行。

- 与 actor 本地状态交互的代码只在该 actor 上运行。

- 一个 actor 一次只运行一部分代码。

由于这些保证，actor 内部不包含 `await` 的代码可以在没有其他代码观察到暂时无效状态风险的情况下进行更新。例如，下面的代码把测得的温度从华氏度转换为摄氏度：

```swift
extension TemperatureLogger {
    func convertFahrenheitToCelsius() {
        for i in measurements.indices {
            measurements[i] = (measurements[i] - 32) * 5 / 9
        }
    }
}
```

上面的代码逐个转换测量值数组中的元素。虽然这个转换操作正在进行，但有些温度是华氏度，另一些是摄氏度。然而，由于代码中没有包含 `await`，这个方法里没有潜在的挂起点。这个方法修改的状态属于这个 actor，actor 会保护它，除非代码运行在 actor 上，否则不能读取或修改它。这意味着在单位转换进行期间，其他代码无法读取部分转换后的温度列表。

除了在 actor 中通过省略潜在的挂起点来保护暂时无效的状态之外，你还可以把这段代码移到一个同步方法中。上面的 `convertFahrenheitToCelsius()` 方法是一个同步方法，因此它保证*绝不*包含潜在的挂起点。这个函数封装了暂时让数据模型不一致的代码，使阅读代码的人更容易认识到，在完成工作恢复数据一致性之前，没有其他代码可以运行。Swift 在这段时间内不会从这段代码切换到程序的另一部分运行代码，这一点很重要。将来，如果你试图向这个函数添加并发代码，引入一个可能的挂起点，你会得到一个编译时错误，而不是引入一个 bug。

## 全局 actor {#Global-Actors}

主 actor 是 [`MainActor`][] 类型的全局单例实例。一个 actor 通常可以有多个实例，每个实例提供独立的隔离。这就是为什么你要把一个 actor 的所有隔离数据声明为该 actor 的实例属性。然而，由于 `MainActor` 是单例——该类型永远只有一个实例——仅凭这个类型就足以标识该 actor，让你可以只用一个特性来标记主 actor 隔离。这种方法让你可以更灵活地按照最适合自己的方式组织代码。

[`MainActor`]: https://developer.apple.com/documentation/swift/mainactor

你可以使用 `@globalActor` 特性定义自己的单例全局 actor，详见 [globalActor](attributes.md#globalActor)。

## 可发送类型 {#Sendable-Types}

任务和 actor 让你可以把程序划分为可以安全并发运行的部分。在任务或 actor 实例内部，程序中包含可变状态（如变量和属性）的部分被称为*并发域*（concurrency domain）。某些类型的数据不能在并发域之间共享，因为这些数据包含可变状态，却没有防止重叠访问。

可以在并发域之间共享的类型被称为*可发送类型*（sendable type）。例如，它可以在调用 actor 方法时作为实参传入，或作为任务的结果返回。本章前面的示例没有讨论可发送性，因为这些示例使用的是简单的值类型，对于在并发域之间传递的数据来说，它们始终可以安全地共享。相比之下，有些类型不能安全地跨并发域传递。例如，一个包含可变属性且没有序列化对这些属性的访问的类，在你把该类的实例在不同任务之间传递时，可能会产生不可预测且错误的结果。

你通过声明对 `Sendable` 协议的一致性，把一个类型标记为可发送。该协议没有任何代码要求，但它有 Swift 会强制执行的语义要求。一般来说，一个类型可以通过三种方式成为可发送类型：

- 该类型是值类型，其可变状态由其他可发送数据组成——例如，带有可发送存储属性的结构体，或带有可发送关联值的枚举。
- 该类型没有任何可变状态，其不可变状态由其他可发送数据组成——例如，只有只读属性的结构体或类。
- 该类型有代码来确保其可变状态的安全性，例如标记为 `@MainActor` 的类，或在特定线程或队列上序列化对其属性访问的类。

关于语义要求的详细列表，参见 [`Sendable`](https://developer.apple.com/documentation/swift/sendable) 协议参考。

有些类型始终是可发送的，例如只包含可发送属性的结构体，以及只包含可发送关联值的枚举。例如：

```swift
struct TemperatureReading: Sendable {
    var measurement: Int
}

extension TemperatureLogger {
    func addReading(from reading: TemperatureReading) {
        measurements.append(reading.measurement)
    }
}

let logger = TemperatureLogger(label: "Tea kettle", measurement: 85)
let reading = TemperatureReading(measurement: 45)
await logger.addReading(from: reading)
```

由于 `TemperatureReading` 是只包含可发送属性的结构体，而且该结构体没有标记为 `public` 或 `@usableFromInline`，所以它是隐式可发送的。下面是该结构体的一个版本，其中对 `Sendable` 协议的一致性是被隐式推断出来的：

```swift
struct TemperatureReading {
    var measurement: Int
}
```

要把一个类型显式标记为不可发送，请写一个不可用的 `Sendable` 一致性：

```swift
struct FileDescriptor {
    let rawValue: Int
}

@available(*, unavailable)
extension FileDescriptor: Sendable {}
```

你也可以使用不可用的一致性来抑制对协议的隐式一致性，如 [协议的隐式一致性](protocols.md#Implicit-Conformance-to-a-Protocol) 中所述。
