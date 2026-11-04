# C++ 并发与并行编程：线程、内存模型、并行计算与服务器

从一个进程里的线程讲到同时应付大量连接的服务器：C++20 标准库是主线，pthread、OpenMP、MPI 和 Linux 系统调用
并排对照。主题参考了 C++ Concurrency in Action（第 2 版）、《并行程序设计导论》、《Linux 高性能服务器编程》三本书，
按主题合编；正文、示例和图都是重新写的，并补上这些书出版以后标准和内核的变化。10 部分 42 章，另有 4 个附录。

在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/cpp-concurrency-zh/>

- `src/`：各章 Markdown，`src/SUMMARY.md` 是目录；`src/code/chNN/` 是各章的示例程序，`src/figs/chNN/` 是图。
  示例用 GCC 15（C++20 / C17，`-Wall -Wextra -Wpedantic`）编译并实际运行，多线程示例都在 ThreadSanitizer 下再跑一遍；
  卡死的示例附 gdb 取到的线程栈，MPI 示例用 Open MPI 4 个进程运行。正文引用的输出（`.out` 文件）由检查脚本生成，不是手写的。
- 图是 JSON，由 `theme/fig.js` 在浏览器里画成可以逐步播放的动图；`theme/fig_preprocess.py` 是对应的 mdBook 预处理器，构建时需要 `python3`。
- 网页由仓库的 GitHub Actions 用 mdBook 构建，本目录不含生成的 HTML。

## 许可证

- 文字和图：CC BY 4.0（[LICENSE](LICENSE)）
- 示例代码：MIT（[LICENSE-CODE](LICENSE-CODE)）
