# AI Agent 工程：从一次模型调用到多 agent 系统

先直接调模型 API，从一次请求讲到几十行的 agent 循环；再讲 workflow 模式、工具设计、MCP、上下文工程、多 agent、
护栏与评估、harness 与生产，最后用 LangGraph 和 OpenAI Agents SDK 把前面手写的东西重写一遍做对照。
内容按主题合编了 Anthropic、OpenAI 的 agent 工程文章，MCP 文档和规范（2026-07-28），Agent Skills 规范，
LangChain Academy 和 LangChain 文档；正文、示例和图都是重新写的，并按 2026 年 11 月的规范、文档和实际运行结果核对过。
资料清单见前言，每篇讲了什么、哪些已经过时见附录 C。9 部分 27 章，另有前言和 4 个附录。

在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/ai-agent-zh/>

- `src/`：各章 Markdown，`src/SUMMARY.md` 是目录；`src/code/chNN/` 是各章的示例，`src/code/shop/` 是贯穿全书的虚构书店
  “青石书店”（本地 SQLite 数据、店铺政策、客服工具），`src/code/harness/` 是第 3 章写出来的最小 agent 循环；`src/figs/chNN/` 是图。
- 示例用 Python 3.12，模型走 DeepSeek 的 OpenAI 兼容接口（`deepseek-flash`），205 个示例都实际运行过一次。
  `.out` 是那一次运行的输出，`.cassette.jsonl` 是那一次的全部模型请求和响应，都由录制脚本生成，不是手写的。
  模型每次的回答都不同，自己运行会得到不同但同类的结果。运行方法见 `src/code/README.md`。
- 图是 JSON，由 `theme/fig.js` 在浏览器里画成可以逐步播放的动图；`theme/fig_preprocess.py` 是对应的 mdBook 预处理器，构建时需要 `python3`。
- 网页由仓库的 GitHub Actions 用 mdBook 构建，本目录不含生成的 HTML。

## 许可证

- 文字和图：CC BY 4.0（[LICENSE](LICENSE)）
- 示例代码：MIT（[LICENSE-CODE](LICENSE-CODE)）
