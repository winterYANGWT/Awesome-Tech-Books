# 附录 C 副作用查看器：原理、局限和用法

书里每个“逐行执行，看副作用”背后都是一次真实的 CMake 运行。这一篇说明它是怎么录下来的、哪些东西它看不到，以及怎样把它用在你自己的项目上。

## C.1 四份数据，四页

| 页 | 数据来源 | 录制器做了什么 |
|---|---|---|
| 配置 | CMake 自带的调试器（`--debugger`，3.27 起） | 在每条命令之前停下，问调用栈、每一层作用域的变量、缓存、目标（类型、源文件、包含目录、宏、选项、链接库和其他属性），再列一遍工作目录里的文件 |
| 配置（参数） | `--trace-expand --trace-format=json-v1` | 每条命令展开变量之后实际收到的参数、命令跨了几行、两次停下之间在 CMake 自带模块里执行了多少条命令 |
| 生成 | CMake 的 file API（`codemodel` v2） | 配置结束后每个目标最终的编译参数、包含目录、宏、语言标准、链接参数、依赖和安装位置，每一项都带回溯：哪个文件哪一行的哪条命令加的 |
| 构建、运行 | `cmake --build build --verbose -j 1`，以及示例里写的后续命令 | 每条真实命令和输出；命令执行前后各列一次文件，差别就是它改动的文件 |

调试器用的是 DAP（Debug Adapter Protocol），和 VS Code 调试 C++ 程序用的是同一套协议。录制器做的事和你在编辑器里“单步执行、看变量窗口”一样，只是每一步都把变量窗口完整地存了下来。

“这一行做了什么”是相邻两次停下之间的差：第 k 步的副作用 = 第 k+1 次停下时的状态 − 第 k 次停下时的状态。所以它看到的是**结果**，不是 CMake 内部做了什么动作。

## C.2 它看不到的东西

| 情况 | 原因 | 在哪里能看到 |
|---|---|---|
| 导入目标（`IMPORTED`）和别名（`ALIAS`）的属性 | 调试器根本不列出这两种目标；录制器从命令记录里还原它们：创建它们的命令和 `set_target_properties`、`set_property(TARGET …)`、`target_*(… INTERFACE …)` | 查看器注明“属性是从执行过的命令里还原的”；以生成页为准 |
| 父目录修改子目录里定义的目标（例如 `target_link_libraries(子目录的目标 …)`） | 调试器只列出当前目录的目标；录制器只能保留上次在子目录里看到的样子 | 生成页的最终结果是对的 |
| 函数体最后一行对函数局部变量的修改 | 下一次停下时已经回到调用者，函数作用域没了 | 它对外层作用域的修改（`PARENT_SCOPE`）看得到 |
| `else()`、`endif()`、`endfunction()` 这类结束标记 | 它们不是会执行的命令，调试器不在这里停 | 下一步跳到哪一行说明了走了哪个分支 |
| `project()`、`enable_language()`、`try_compile()` 内部 | 录制器跨过这几条命令，只看结果 | 第 2 章讲 `project()` 的内部 |
| 策略（policy）的状态 | 策略不是变量，调试器不提供 | `cmake_policy(GET …)` 读出来，或者看第 23 章 |
| 生成器表达式在配置页里的值 | 这时还没有求值，属性里存的就是 `$<…>` 原文 | 生成页 |

另外两个录制器自己的约定：每个 `CMakeLists.txt` 的拷贝末尾多了一行 `return()`，让录制器能在文件结束处停一次（查看器显示为“文件执行完”）；文件 API 的查询文件和回复目录在文件列表里被隐藏。

## C.3 用在你自己的项目上

```sh
python3 tools/sidefx/sidefx.py ~/code/mylib                       # 配置 + 构建，写出 ~/code/mylib.sidefx.html
python3 tools/sidefx/sidefx.py ~/code/mylib --no-build            # 只配置
python3 tools/sidefx/sidefx.py ~/code/mylib -- -DBUILD_TESTING=ON # 额外的 cmake 参数写在 -- 后面
python3 tools/sidefx/sidefx.py ~/code/mylib --serve 3001          # 同时在 3001 端口提供这个页面
```

命令在本书源码目录下运行（取法见[前言](index.md)），`sidefx.py` 会读同一目录里 `theme/` 下的查看器，所以要整个目录一起取。要求 CMake 3.27 以上（调试器）和 Ninja（录制器默认 `-G Ninja`）。项目先被复制到一个临时目录再运行，你的源码和已有的构建目录都不会被改动；录下来的路径里，临时目录统一写成 `/work`。

工程很大时，录制会慢（每一步都要列一遍文件、读一遍全部变量），页面也会很大。这时可以只录你关心的那一部分：把要研究的子目录单独拿出来，或者写一个小的 `sidefx.json`（格式见下节）。

## C.4 `sidefx.json`

放在项目根目录。没有它时只有一次运行：配置，然后构建。

```json
{
  "title": "改缓存再配置",
  "runs": [
    {"label": "第一次"},
    {"label": "关掉 FAST", "args": ["-DUSE_FAST=OFF"], "after": [["./build/app"]]}
  ],
  "step_into": ["prefix/lib/cmake"],
  "contents": ["build/build.ninja"]
}
```

| 字段 | 含义 |
|---|---|
| `runs[].args` | 额外的 cmake 参数 |
| `runs[].src` / `runs[].build` | 工程子目录（默认 `.`）和构建目录名（默认 `build`），用来录“先装一个库、再配置使用它的工程” |
| `runs[].fresh` | 这次运行前删掉构建目录 |
| `runs[].replace` | 运行前用别的文件替换某个文件，演示“改了 CMakeLists.txt 再配置” |
| `runs[].configure` | `false` 时不单独执行 cmake，只构建：用来看构建时的自动重新配置 |
| `runs[].build_targets` | `false` 不构建；列表则只构建这些目标 |
| `runs[].after` | 构建后在工作目录里执行的命令，例如 `["./build/app"]`、`["ctest","--test-dir","build"]`、`["cmake","--install","build","--prefix","prefix"]` |
| `runs[].expect` | `"ok"`、`"configure-error"` 或 `"build-error"`：录反例时用 |
| `step_into` | 除了工程自己的目录，还要逐行进入哪些目录里的 `.cmake` 文件，例如安装前缀里的包配置文件 |
| `contents` | 默认只保存小于 24 KB、不在 `CMakeFiles/` 下的文本文件内容；要看别的文件写在这里 |
| `env` | 额外的环境变量 |

## C.5 录制文件的结构

页面读的是一份 JSON。想自己分析时，主要的字段：

| 位置 | 内容 |
|---|---|
| `runs[i].configure.steps[k]` | 第 k+1 步：`file`、`line`、`cmd`、`args`（展开后）、`stack`（外层调用）、`scopeLabel`、`fx`（这一步的副作用） |
| `steps[k].fx` | `vars`（按作用域分组的变量变化，`top` 表示当前作用域）、`cache`、`targets`、`files`、`out`、`entered` / `left` / `dropped`（作用域的进入、离开和销毁） |
| `runs[i].configure.state0` | 第一步之前的状态（缓存在第二次配置时就不是空的） |
| `runs[i].configure.generate.files` | 生成阶段写出的文件 |
| `runs[i].configure.codemodel.targets` | file API 给出的每个目标的最终参数和回溯 |
| `runs[i].build_result.commands` | 构建命令 |
| `runs[i].after` | 后续命令的输出和文件变化 |
| `blobs` | 文件内容，按哈希存一份 |

本书源码目录里的 `tools/sidefx/sidefx_text.py` 把一份录制打印成文字，每步一行，适合在终端里查看或拿去比较两次录制。
