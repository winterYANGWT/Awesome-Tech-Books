# 关于语言参考 {#About-the-Language-Reference}

阅读形式语法所使用的记法。

本书的这一部分描述 Swift 编程语言的形式语法。这里描述的语法旨在帮助你更详细地理解这门语言，而不是让你直接实现解析器或编译器。

Swift 语言本身相对精简，因为 Swift 代码中几乎无处不在的许多常见类型、函数和运算符实际上是在 Swift 标准库中定义的。尽管这些类型、函数和运算符不属于 Swift 语言本身，但本书这一部分的论述和代码示例中大量使用了它们。

## 如何阅读语法 {#How-to-Read-the-Grammar}

用于描述 Swift 编程语言形式语法的记法遵循若干约定：

- 箭头（→）用于标记语法产生式，可读作“由……构成”。
- 语法范畴用*斜体*文字表示，出现在语法产生式规则的左右两侧。
- 字面词和标点用 **`粗体等宽`** 文字表示，只出现在语法产生式规则的右侧。
- 备选的语法产生式之间用竖线（|）分隔。当备选产生式太长而不易阅读时，会换行拆分成多个语法产生式规则。
- 少数情况下，会用常规字体文字描述语法产生式规则的右侧。
- 可选的语法范畴和字面量用末尾的问号 *?* 标记。

作为示例，getter-setter 块的语法定义如下：

> getter-setter 块语法：
>
> *getter-setter-block* → **`{`** *getter-clause* *setter-clause*_?_ **`}`** | **`{`** *setter-clause* *getter-clause* **`}`**

这个定义表明，getter-setter 块可以由花括号括起来的 getter 子句后跟一个可选的 setter 子句构成，*或者*由花括号括起来的 setter 子句后跟一个 getter 子句构成。上面的语法产生式等价于下面两个产生式，其中备选项被明确列出：

> getter-setter 块语法：
>
>
> *getter-setter-block* → **`{`** *getter-clause* *setter-clause*_?_ **`}`** \
> *getter-setter-block* → **`{`** *setter-clause* *getter-clause* **`}`**
