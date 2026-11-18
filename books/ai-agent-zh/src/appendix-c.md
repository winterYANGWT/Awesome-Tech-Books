# 附录 C 资料来源：每篇讲了什么，哪些已经过时

这两张表按缩写列出全书用到的资料：先看每篇讲了什么、哪些说法要改，再从“书里位置”回到正文对应小节。引用文章时只转述，不复制原文。

## 来源总表

| 缩写 | 标题 / 来源 | 日期 | 讲了什么 | 今天要改的地方 | 书里位置 |
|---|---|---|---|---|---|
| BEA | Anthropic《Building effective agents》 | 2024-12-19 | workflow 与 agent 的区分、五种 workflow 模式、ACI（agent-computer interface）、客服和编程 agent 的实践 | 框架清单和“直接用 MCP 客户端”是当时的；模型只当“小模型 / 强模型”的例子，不写型号 | [1.4 节](ch01.md)、[3.1 节](ch03.md)、[5.1 节](ch05.md)、[6.1 节](ch06.md)、[7.1 节](ch07.md)、[8.1 节](ch08.md)、[9.5 节](ch09.md) |
| OPG | OpenAI《A practical guide to building agents》 | 2025-04 | 模型 / 工具 / 指令三件套、单 agent 循环、manager / handoff、护栏、人工介入 | 代码有多处错误，按 Agents SDK 0.24 重写；o1 / o3-mini 型号已旧；`output_type` 在 DeepSeek 上不可用 | [1.4 节](ch01.md)、[2.5 节](ch02.md)、[3.6 节](ch03.md)、[17.1 节](ch17.md)、[19.5 节](ch19.md)、[20.1 节](ch20.md)、[27.1 节](ch27.md)、[27.3 节](ch27.md)、[27.4 节](ch27.md) |
| WTA | Anthropic《Writing effective tools for agents — with agents》 | 2025-09-11 | 工具设计原则、用评估改进工具、返回值 token 预算 | “25,000 token 上限”“Sonnet 3.5 的 SWE-bench”是当时数字；正文只讲按需截断、分页和过滤 | [8.2 节](ch08.md)、[8.4 节](ch08.md)、[9.1 节](ch09.md)、[9.3 节](ch09.md)、[9.4 节](ch09.md)、[21.1 节](ch21.md) |
| ECE | Anthropic《Effective context engineering for AI agents》 | 2025-09 | 上下文是有限资源、即时检索、压缩、笔记、子 agent | 当时的 200k 量级上下文（MAR）、“memory tool 公测”是当时的产品状态；改成通用说法，产品状态注明“2026-11 时” | [13.2 节](ch13.md)、[14.1 节](ch14.md)、[15.2 节](ch15.md)、[15.5 节](ch15.md)、[22.2 节](ch22.md) |
| SKL | Anthropic《Equipping agents for the real world with Agent Skills》 | 2025-10-16；2025-12-18 更新 | Skill 的结构、三层渐进式披露、脚本、评估、安全 | 2025-12 起 Skills 成为开放标准（SKS）；平台细节不写，以更新版为准 | [16.1 节](ch16.md)、[16.3 节](ch16.md)、[16.4 节](ch16.md)、[16.6 节](ch16.md)、[16.7 节](ch16.md)、[19.6 节](ch19.md) |
| MAR | Anthropic《How we built our multi-agent research system》 | 2025-06-13 | 编排者-工作者、委派提示、评估、生产可靠性 | “90.2%”“80% 方差”“15 倍 token”是 Anthropic 内部评测，引用时注明来源；200k 是当时的窗口 | [7.6 节](ch07.md)、[18.1 节](ch18.md)、[18.2 节](ch18.md)、[18.6 节](ch18.md)、[21.4 节](ch21.md)、[23.4 节](ch23.md)、[23.5 节](ch23.md) |
| MAN | Anthropic《Scaling Managed Agents: Decoupling the brain from the hands》 | 2026-04-08 | session / harness / sandbox 三分、无状态 harness、凭据隔离、上下文焦虑 | 平台实现细节只讲原理；不写具体产品名；p50 / p95 是内部数据，引用时注明 | [12.3 节](ch12.md)、[15.2 节](ch15.md)、[19.6 节](ch19.md)、[22.1 节](ch22.md)、[22.3 节](ch22.md)、[22.6 节](ch22.md) |
| MCP | MCP 规范与文档 2026-07-28 | 2026-07-28 | JSON-RPC、三类原语、传输、授权、MRTR（多轮请求）、缓存、安全 | 2026-07-28 去掉了握手和协议层会话；旧教程以旧版为准；Python SDK 2.x 改名 `MCPServer` | [10.2 节](ch10.md)、[10.3 节](ch10.md)、[10.4 节](ch10.md)、[11.1 节](ch11.md)、[11.4 节](ch11.md)、[12.1 节](ch12.md)、[12.2 节](ch12.md) |
| SKS | Agent Skills 开放标准（agentskills.io） | 2026-11 下载 | `SKILL.md` 格式、客户端实现指南、评估、脚本 | 与 SKL 的 2025-12 更新对齐；客户端实现细节按标准页，平台差异不写成协议 | [16.1 节](ch16.md)、[16.2 节](ch16.md)、[16.6 节](ch16.md)、[16.7 节](ch16.md) |
| LCA | LangChain Academy《Introduction to LangGraph》 | 仓库 2026-06 | LangGraph 的图、状态、记忆、人工介入、子图、部署 | 以 `gpt-4o` 为主（少量 `gpt-3.5-turbo`）；agent 用 `ToolNode` + `tools_condition` 手搭（v1 仍可用）；记忆示例的 trustcall、Tavily、uuid 要换成本地做法 | [5.6 节](ch05.md)、[13.4 节](ch13.md)、[20.6 节](ch20.md)、[23.5 节](ch23.md)、[24.4 节](ch24.md)、[25.2 节](ch25.md)、[26.4 节](ch26.md) |
| LCD | LangChain 文档 `oss/python` 部分 | 2026-11 下载 | LangGraph v1、LangChain v1 的现行 API | 个别页仍用 `MemorySaver` 旧名；以页面正文和实测为准；`recursion_limit` 默认值按 1.2.22 | [24.2 节](ch24.md)、[24.5 节](ch24.md)、[25.1 节](ch25.md)、[25.5 节](ch25.md)、[26.1 节](ch26.md)、[26.4 节](ch26.md)、[27.6 节](ch27.md) |

## 旧说法 → 现在

| 旧说法 | 现在 | 影响 |
|---|---|---|
| BEA 2024-12 列的框架清单、直接用 MCP 客户端接生态 | 先用模型 API 手写循环；MCP 以 2026-07-28 无状态版为准 | [1.4 节](ch01.md)、[10.4 节](ch10.md)、[11.1 节](ch11.md) |
| BEA 用 Haiku 4.5、Sonnet 4.5 举例 | 只讲“小模型 / 强模型”；示例用 `deepseek-flash`，对照用 `deepseek-v4-pro` | [1.4 节](ch01.md)、[5.5 节](ch05.md) |
| OPG 的 `Agents.run(agent, [UserMessage(...)])` | 0.24 用 `Runner.run_sync(agent, "…")` 或 `await Runner.run(...)` | [3.6 节](ch03.md)、[17.5 节](ch17.md)、[27.1 节](ch27.md) |
| OPG 的 `name=Triage Agent"`、未定义的 `agent`、`datetime.time()`、`math_homework_tripwire` | 按 0.24 重写，示例不复制这些错误 | [27.1 节](ch27.md) |
| OPG 的 `Guardrail(guardrail_function=...)` | 0.24 用 `InputGuardrail(...)` 或 `@input_guardrail`；`agents` 没有 `Guardrail` 类 | [27.4 节](ch27.md) |
| OPG 用 `output_type` 做结构化输出 | DeepSeek 的 Chat Completions 不支持 `json_schema`，会返回 400；Agents SDK 走默认的 Responses 模式时可用，或改用终止工具 | [4.5 节](ch04.md)、[19.4 节](ch19.md)、[27.3 节](ch27.md) |
| OPG 推荐 o1 / o3-mini 生成指令 | 不写具体型号，只讲“用强模型生成、再用评估筛” | [2.5 节](ch02.md)、[5.6 节](ch05.md) |
| WTA 的“25,000 token 工具返回上限”、Sonnet 3.5 的 SWE-bench 数字 | 只讲“按需截断、分页、范围过滤”，不引用具体数字 | [9.3 节](ch09.md)、[9.6 节](ch09.md) |
| ECE 的 memory tool 公测、MAR 的 200k 上下文和具体模型名 | 改成通用说法；产品状态注明“2026-11 时” | [2.6 节](ch02.md)、[13.1 节](ch13.md)、[15.6 节](ch15.md)、[22.2 节](ch22.md) |
| MAR 的“多 agent 比单 agent 强 90.2%”“token 解释 80% 方差”“约 15 倍 token” | 注明是 Anthropic 内部评测，不作为普遍规律 | [7.6 节](ch07.md)、[18.1 节](ch18.md)、[18.5 节](ch18.md) |
| Skill 只是 Claude 的专有功能 | 2025-12 起 Skills 成为开放标准（SKS）；客户端自己实现发现和激活 | [16.7 节](ch16.md) |
| MCP 2024–2025 的 `initialize` 握手、`Mcp-Session-Id`、协议级会话 | 2026-07-28 无状态：每请求 `_meta` 带 `io.modelcontextprotocol/protocolVersion` 和 `io.modelcontextprotocol/clientCapabilities`；新增 `server/discover` | [10.4 节](ch10.md)、[11.1 节](ch11.md)、[12.1 节](ch12.md) |
| MCP 的 `ping`、`logging/setLevel`、`resources/subscribe`、HTTP GET 流和 `Last-Event-ID` 续传 | `ping`、`logging/setLevel` 移除；日志改为每请求 `_meta` 的 `io.modelcontextprotocol/logLevel`，但整个 Logging 功能在 2026-07-28 已弃用（新实现写 stderr 或 OpenTelemetry）；订阅用 `subscriptions/listen`；断流后重发新请求（新 id），不支持 `Last-Event-ID` 续传 | [10.3 节](ch10.md)、[10.4 节](ch10.md)、[11.1 节](ch11.md)、[附录 D](appendix-d.md) |
| MCP 核心里的 tasks | 移到 `io.modelcontextprotocol/tasks` 扩展，用 `tasks/get`、`tasks/update` 轮询 | [附录 D](appendix-d.md) |
| MCP Python SDK 的 `FastMCP` | 2.x 改名 `MCPServer`（`from mcp.server.mcpserver import MCPServer`），字段 snake_case | [11.1 节](ch11.md)、[11.4 节](ch11.md) |
| LCA 以 `gpt-4o` 为主 | `ChatOpenAI(model=MODEL)`，模型名读环境变量 | [5.6 节](ch05.md)、[24.4 节](ch24.md)、[24.5 节](ch24.md) |
| 旧教程用 `langgraph.prebuilt.create_react_agent`（LCA 本身没用） | LangGraph v1 弃用，改用 `langchain.agents.create_agent` | [24.5 节](ch24.md) |
| LCA 用 `MemorySaver` | 现在文档主用 `InMemorySaver`，`MemorySaver` 是别名 | [24.5 节](ch24.md)、[25.1 节](ch25.md)、[26.6 节](ch26.md) |
| LCA 用 `interrupt_before=["tools"]`、`NodeInterrupt` | 条件审批用 `interrupt()` + `Command(resume=...)`；静态断点仍保留 | [25.2 节](ch25.md)、[20.6 节](ch20.md) |
| LCA 的部署模块（LangGraph Platform） | 本机没有 LangSmith 账号，只讲原理，不写部署示例 | [23.5 节](ch23.md) |
| LangGraph 旧文档说 `recursion_limit` 默认 25 | 1.2.22 实测默认 10007（`LANGGRAPH_DEFAULT_RECURSION_LIMIT` 可改） | [24.6 节](ch24.md) |
| Anthropic Messages 的 `max_tokens` 可省略 | 现在必填 | [2.5 节](ch02.md)、[附录 A](appendix-a.md) |
| OpenAI 的 `functions` / `function_call` | 现在 `tools` / `tool_calls` | [3.1 节](ch03.md)、[附录 A](appendix-a.md) |

单条的旧资料提示仍放在各章正文；这里只做汇总。

## 延伸阅读

- Anthropic，《Building effective agents》，2024-12：[链接](https://www.anthropic.com/engineering/building-effective-agents)。workflow 和 agent 的区分、五种 workflow 模式。
- OpenAI，《A practical guide to building agents》，2025-04：[链接](https://cdn.openai.com/business-guides-and-resources/a-practical-guide-to-building-agents.pdf)。模型、工具、指令和单 agent 循环。
- Anthropic，《Writing effective tools for agents — with agents》，2025-09：[链接](https://www.anthropic.com/engineering/writing-tools-for-agents)。工具设计和用评估改进工具。
- Anthropic，《Effective context engineering for AI agents》，2025-09：[链接](https://www.anthropic.com/engineering/effective-context-engineering-for-ai-agents)。上下文腐化、压缩、笔记和子 agent。
- Anthropic，《How we built our multi-agent research system》，2025-06：[链接](https://www.anthropic.com/engineering/multi-agent-research-system)。编排者-工作者、委派和评估。
- Anthropic，《Scaling Managed Agents》，2026-04：[链接](https://www.anthropic.com/engineering/managed-agents)。session、harness、sandbox 的分工。
