# C++ 规则新编

把八本经典 C++ 规则书（Effective C++、More Effective C++、Effective STL、Effective Modern C++、
Exceptional C++、More Exceptional C++、Exceptional C++ Style、C++ Coding Standards）的 410 条规则
按主题合并重讲，并以 C++20 为基准，补上 C++11 到 C++26 的语言变化和今天的工程实践。8 部分 36 章。

在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/cpp-rules-zh/>

- `src/`：各章 Markdown，`src/SUMMARY.md` 是目录；`src/code/chNN/` 是各章的示例程序。
  每个示例都用 Clang 21（`-std=c++20 -Wall -Wextra -Wpedantic -Werror`）编译，并在 AddressSanitizer
  和 UndefinedBehaviorSanitizer 下运行；正文引用的输出（同名 `.out` 文件）由检查脚本生成，不是手写的。
- 网页由仓库的 GitHub Actions 用 mdBook 构建，本目录不含生成的 HTML。
- 许可证：文字 CC BY 4.0（见 `LICENSE`），示例代码 MIT（见 `LICENSE-CODE`）。本目录不适用仓库根目录的 LICENSE。
- 本书是原创的中文写作，不是上述任何一本书的翻译或摘编，也未经其作者或出版社审阅、认可。
  书中用条目编号（如 EC-04）指向原书，引用原文最多一句；书名和商标归各自的权利人所有。
- 本书在 AI 辅助下编写和整理，欢迎通过 issue 指正错误。
