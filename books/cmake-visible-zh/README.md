# 看得见的 CMake

CMake 的副作用大多看不见：变量活在某个作用域里，缓存悄悄写进文件，属性挂在目标上，生成器表达式要到生成阶段才求值，命令到构建时才执行。
这本书的每个示例都真实运行过一遍：CMake 在自己的调试器下逐条命令停下，录下当时的变量、作用域、缓存、目标属性和磁盘上的文件。
读者在书里点开弹出的查看器，逐行回放配置、生成、构建、运行四个阶段留下的副作用；生成页里的每个编译参数都能追回加上它的那一行。
8 部分 26 章，另有前言和 4 个附录。

在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/cmake-visible-zh/>

- `src/`：各章 Markdown，`src/SUMMARY.md` 是目录；`src/figs/chNN/` 是图。
- `src/code/chNN/`：各章的示例，每个是一个 CMake 工程。`sidefx.json` 写明怎样运行它：配置几次、带什么参数、构建后执行什么命令、输出里应该出现什么。
- `src/sidefx/chNN/`：每个示例那一次运行的录制，由录制器生成，不是手写的。录制环境是 Ubuntu、CMake 4.2.3、Ninja 1.13、GCC 15.2，路径统一写成 `/work`；换版本运行，变量个数和编译参数的细节会不同。
- `tools/sidefx/`：录制器 `record.py`；独立入口 `sidefx.py`，把你自己的项目录下来，生成一个可以离线打开的查看器页面；`sidefx_text.py` 把录制打印成文字。只用 Python 标准库，需要 CMake 3.27 以上和 Ninja，在本目录下运行，用法和局限见附录 C。
- `theme/`：`sidefx.js`、`sidefx.css` 是查看器，`sidefx_preprocess.py` 把正文里的启动标记换成启动条；图是 JSON，由 `fig.js` 画成可以逐步播放的动图，`fig_preprocess.py` 是对应的预处理器。两个预处理器构建时都需要 `python3`。
- 正文参考了 CMake 官方文档、*Mastering CMake*、*Modern CMake* 等资料（清单见附录 D）。文字、示例和图都是重新写的，事实按 CMake 4.2.3 的文档和实际录制核对过。
- 网页由仓库的 GitHub Actions 用 mdBook 构建，本目录不含生成的 HTML。

## 许可证

- 文字和图：CC BY 4.0（[LICENSE](LICENSE)）
- 示例代码和 `tools/` 下的工具：MIT（[LICENSE-CODE](LICENSE-CODE)）
