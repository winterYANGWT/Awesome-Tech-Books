# Awesome Tech Books

[![Pages](https://github.com/winterYANGWT/Awesome-Tech-Books/actions/workflows/pages.yml/badge.svg)](https://github.com/winterYANGWT/Awesome-Tech-Books/actions/workflows/pages.yml)

精选技术书籍的中文版，在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/>

## 书目

| 书名 | 说明 | 许可证 | 目录 |
| --- | --- | --- | --- |
| [Rust 程序设计语言](https://winteryangwt.github.io/Awesome-Tech-Books/rust-book-zh/) | [The Rust Programming Language](https://doc.rust-lang.org/book/) 的简体中文译本，译自 2026-09 上游版本（`1500248d`），21 章 + 7 个附录 | MIT / Apache-2.0 | [`rust-book-zh/`](rust-book-zh/) |
| [CUDA 编程：从并行模型到工程实践](https://winteryangwt.github.io/Awesome-Tech-Books/cuda-programming-zh/) | 原创中文教程，每章围绕可运行的程序和实测数据，7 部分 36 章；不是 NVIDIA 官方文档 | 文字 CC BY 4.0，代码 MIT | [`books/cuda-programming-zh/`](books/cuda-programming-zh/) |
| [CUDA 编程：从并行模型到工程实践（旧版）](https://winteryangwt.github.io/Awesome-Tech-Books/cuda-programming-zh-legacy/) | 新版的前身，14 部分 87 章，按主题逐项讲解，每章附 API 速查；已不再更新，留作参考 | 文字 CC BY 4.0，代码 MIT | [`books/cuda-programming-zh-legacy/`](books/cuda-programming-zh-legacy/) |
| [C++ 规则新编](https://winteryangwt.github.io/Awesome-Tech-Books/cpp-rules-zh/) | 原创中文教程，把八本经典 C++ 规则书的 410 条规则按主题合并重讲，以 C++20 为基准补上新标准和工程实践，8 部分 36 章，512 个示例均用 Clang 21 实测 | 文字 CC BY 4.0，代码 MIT | [`books/cpp-rules-zh/`](books/cpp-rules-zh/) |
| [Swift 编程语言](https://winteryangwt.github.io/Awesome-Tech-Books/swift-book-zh/) | [The Swift Programming Language](https://docs.swift.org/swift-book/) 的简体中文译本（Swift 6.4），译自 2026-09 上游版本（`1c05984`），语言指南 + 语言参考共 43 章 | Apache-2.0（附 Runtime Library Exception） | [`books/swift-book-zh/`](books/swift-book-zh/) |
| [容器与 Kubernetes 实践](https://winteryangwt.github.io/Awesome-Tech-Books/container-k8s-zh/) | 原创中文教程，以 Kubernetes 1.37 / Docker Engine 29 官方文档为准，从 namespace、cgroup 讲到集群运维，9 部分 36 章，278 个示例均在 Docker-in-Docker 实验环境实测 | 文字 CC BY 4.0，代码 MIT | [`books/container-k8s-zh/`](books/container-k8s-zh/) |
| [四足机器人控制：从关节电机到自主行走](https://winteryangwt.github.io/Awesome-Tech-Books/quadruped-control-zh/) | 原创中文教程，参考《四足机器人控制算法——建模、控制与实践》的章节结构、覆盖其全部 11 章内容，从关节电机讲到小跑和导航，70 个示例用 MuJoCo 仿真 Unitree A1 实测 | 文字 CC BY 4.0，代码 MIT | [`books/quadruped-control-zh/`](books/quadruped-control-zh/) |

这些书都借助 AI 翻译或编写，难免有错，欢迎通过 issue 指正。

## 仓库结构

```
index.html                   站点首页，每本书一张卡片
rust-book-zh/                Rust 书：已构建好的 HTML，原样发布
books/<name>/                mdBook 源码（book.toml、src/、LICENSE* 等），构建到站点的 <name>/
reader/                      各书共用的阅读器：内容宽度设置、阅读记录
scripts/build-site.sh        把首页和各书组装成完整站点
scripts/inject-reader.py     构建时把 reader/ 注入各书的阅读页
.github/workflows/pages.yml  推送到 main 时构建站点并发布到 GitHub Pages
```

## 阅读功能

所有书的阅读页都能调内容宽度（菜单栏右侧的设置按钮），并记录看过的页面和每页的位置：再次进入一本书时回到上次的页面和位置，目录里看过的页面带 ✓。这些数据只保存在读者自己浏览器的 localStorage 里，不会上传，换设备或浏览器不同步。

这些功能在构建时注入，所以 `mdbook build` 单独构建一本书时没有，要用 `scripts/build-site.sh` 预览。

## 构建与发布

推送到 `main` 后由 GitHub Actions 构建并发布，生成的 HTML 不进 git。仓库的 Settings → Pages → Source 须设为 "GitHub Actions"。

本地预览需要 PATH 上有 [mdBook](https://github.com/rust-lang/mdBook) 0.5.1（与 CI 保持一致）：

```sh
scripts/build-site.sh                  # 输出到 _site/，已被 .gitignore 忽略
python3 -m http.server -d _site 8000   # 然后打开 http://localhost:8000/
```

## 新增一本书

1. 把 mdBook 源码放到 `books/<name>/`，并附上至少一个 `LICENSE*` 文件：构建脚本会把它复制进站点，缺了会构建失败。不要提交 mdBook 生成的 `book/` 目录。
2. 在 `index.html` 里复制一张 `<article class="book">` 卡片，改强调色、文字和链接，“开始阅读”指向 `<name>/`。
3. 在上面的书目表里加一行，推送到 `main`。

## 许可证

根目录的 [LICENSE](LICENSE)（MIT）只适用于站点本身：首页、脚本和工作流。各书的许可证见其目录中的 LICENSE 文件。
