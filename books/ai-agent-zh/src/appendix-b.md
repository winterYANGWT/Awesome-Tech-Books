# 附录 B 术语表

正文第一次出现术语时给英文；这里按章汇总，每行链到讲它的那一节。`agent` 不译，查词先看中文再看定义。

| 中文 | 英文 | 一句话定义 | 第一次出现 |
|---|---|---|---|
| agent | agent | 模型在循环里自己决定调用哪个工具、什么时候停 | [1.3 节](ch01.md) |
| workflow | workflow | 代码预先写好路径、模型只填其中一步的系统 | [1.1 节](ch01.md) |
| 增强型 LLM | augmented LLM | 带检索、工具、记忆的模型 | [1.1 节](ch01.md) |
| token | token | 模型计费和上下文长度的单位 | [2.2 节](ch02.md) |
| 上下文窗口 | context window | 一次请求里模型能看到的 token 上限 | [2.1 节](ch02.md) |
| 工具调用 | tool calling / function calling | 模型输出结构化的工具名和参数，由程序执行 | [3.2 节](ch03.md) |
| JSON Schema | JSON Schema | 描述工具参数结构的 JSON 文档 | [3.1 节](ch03.md) |
| `tool_choice` | tool_choice | 控制模型必须/可以/不得调用工具的参数 | [3.1 节](ch03.md) |
| 并行工具调用 | parallel tool calls | 一次回复里返回多个 `tool_calls` | [3.4 节](ch03.md) |
| agent 循环 | agent loop | 调模型→执行工具→回填结果→再调模型，直到停止 | [3.3 节](ch03.md) |
| 最大轮数 | max turns | 循环的硬上限，防止停不下来 | [3.3 节](ch03.md) |
| harness | harness | 驱动模型循环、执行工具的那层程序 | [3.6 节](ch03.md) |
| 结构化输出 | structured output | 让模型按给定 schema 返回 JSON | [4.3 节](ch04.md) |
| JSON 模式 | JSON mode | 只保证输出是合法 JSON，不保证字段 | [4.2 节](ch04.md) |
| 提示链 | prompt chaining | 把任务拆成固定顺序的多次模型调用 | [5.1 节](ch05.md) |
| 路由 | routing | 先分类，再把输入交给不同的提示或模型 | [5.3 节](ch05.md) |
| 并行化 | parallelization | 同时跑多个模型调用，再合并结果 | [6.1 节](ch06.md) |
| 分段 | sectioning | 把任务拆成互不依赖的几段并行做 | [6.1 节](ch06.md) |
| 投票 | voting | 同一问题问多次，按多数或阈值决定 | [6.3 节](ch06.md) |
| 编排者-工作者 | orchestrator-workers | 一个模型拆任务，多个 worker 执行，再汇总 | [7.1 节](ch07.md) |
| 评估者-优化者 | evaluator-optimizer | 一次调用生成，另一次调用只评估并给反馈，循环改进 | [7.4 节](ch07.md) |
| 工具描述 | tool description | 给模型看的工具说明，决定它什么时候选这个工具 | [8.4 节](ch08.md) |
| ACI | agent-computer interface | 工具、返回值、错误信息合起来构成的“给 agent 用的界面” | [8.1 节](ch08.md)、[9.4 节](ch09.md) |
| MCP | Model Context Protocol | 让 agent 接外部工具/资源的开放协议 | [10.1 节](ch10.md) |
| JSON-RPC | JSON-RPC | MCP 的消息格式：`jsonrpc`/`id`/`method`/`params` | [10.2 节](ch10.md) |
| host / client / server | host / client / server | 宿主应用、协议客户端、提供工具/资源/提示的一方 | [10.1 节](ch10.md) |
| 原语 | primitive | MCP 的 tools、resources、prompts 三类能力 | [10.3 节](ch10.md) |
| 会话 | session | 一段有状态的交互；MCP 2026-07-28 已无协议层会话 | [10.4 节](ch10.md) |
| stdio / Streamable HTTP | stdio / Streamable HTTP | MCP 的两种传输 | [11.1 节](ch11.md) |
| 多轮请求 | MRTR | 服务端返回 `input_required`，客户端带答复重试 | [12.1 节](ch12.md) |
| 上下文工程 | context engineering | 决定每一轮把哪些信息放进上下文 | [13.1 节](ch13.md) |
| 上下文腐化 | context rot | 上下文越长，模型对其中信息的利用越差 | [13.2 节](ch13.md) |
| 注意力预算 | attention budget | 模型对上下文中各项的注意力是有限资源 | [13.2 节](ch13.md) |
| 渐进式披露 | progressive disclosure | 先给索引，需要时再加载详细内容 | [14.2 节](ch14.md) |
| 检索增强生成 | RAG | 先检索文档再生成回答 | [14.1 节](ch14.md) |
| 即时检索 | just-in-time retrieval | agent 在循环里自己决定查什么 | [14.1 节](ch14.md) |
| 压缩 | compaction | 把旧对话总结成短摘要，腾出上下文 | [15.2 节](ch15.md) |
| 工具结果清理 | tool result clearing | 把旧的工具输出换成一两行 stub，只留结论和关键字段 | [15.3 节](ch15.md) |
| 结构化笔记 | structured note-taking | 用 `NOTES.md` 等文件记跨轮次的状态 | [15.4 节](ch15.md) |
| 子 agent | subagent | 由主 agent 派出去、带独立上下文的 agent | [15.5 节](ch15.md) |
| 交接 | handoff | 一个 agent 把控制权交给另一个 agent | [17.3 节](ch17.md) |
| manager | manager | 把其他 agent 当工具调用的编排方式 | [17.2 节](ch17.md) |
| 护栏 | guardrail | 在输入、输出或工具调用前后做检查的规则 | [19.6 节](ch19.md) |
| tripwire | tripwire | 护栏命中后抛出的异常/停止信号 | [19.5 节](ch19.md)、[27.4 节](ch27.md) |
| 提示注入 | prompt injection | 把指令藏进用户输入、工具结果或文档里 | [19.1 节](ch19.md) |
| 人工介入 | human-in-the-loop | 让人批准、修改或接管 agent 的动作 | [20.1 节](ch20.md) |
| 中断 | interrupt | 在节点里暂停图，等外部输入再继续 | [20.3 节](ch20.md)、[25.2 节](ch25.md) |
| 评估 / 评测集 | evaluation / eval set | 用一组任务衡量 agent 的表现 | [21.1 节](ch21.md) |
| 以模型为评委 | LLM-as-judge | 用模型按评分表给输出打分 | [21.4 节](ch21.md) |
| 终态评估 | end-state evaluation | 查数据库或文件里的终态，看动作是否真的发生 | [21.2 节](ch21.md) |
| 沙箱 | sandbox | 限制代码执行权限的隔离环境 | [22.4 节](ch22.md) |
| 追踪 | trace | 一次运行里每一步请求、工具调用、token 和耗时的结构化记录 | [23.1 节](ch23.md) |
| 持久执行 | durable execution | 崩溃或重启后从断点继续，不重复副作用 | [23.4 节](ch23.md) |
| 上下文缓存 | context caching | 复用相同前缀的 KV 缓存，降低费用 | [23.3 节](ch23.md) |
| reducer | reducer | 规定同一个 state key 的多次写入怎么合并 | [24.3 节](ch24.md) |
| 状态图 | state graph | 节点读写共享 state、边决定下一步的图 | [24.1 节](ch24.md) |
| 节点 / 边 | node / edge | 图里的执行单元和转移 | [24.2 节](ch24.md) |
| 检查点 | checkpoint | 图在某个超步结束时的状态快照 | [25.1 节](ch25.md) |
| thread | thread | 一组按顺序排列的检查点，用 `thread_id` 标识 | [25.1 节](ch25.md) |
| 时间回溯 | time travel | 从旧检查点重放或分叉 | [25.4 节](ch25.md) |
| 子图 | subgraph | 作为另一个图的节点使用的图 | [26.1 节](ch26.md) |
| `Send` | Send | 条件边按运行时数量扇出到同一个节点 | [26.3 节](ch26.md) |
| Store | store | 跨 thread 的键值存储，用来做长期记忆 | [26.4 节](ch26.md) |
| 命名空间 | namespace | Store 里定位一组键的 tuple | [26.4 节](ch26.md) |

> **别混用**：第 10 章的“会话”是 MCP 协议层的概念，2026-07-28 起已取消，`Mcp-Session-Id` 也不再使用；第 22 章的 session 指 harness 的只追加事件日志（[22.1 节](ch22.md)）。

## 延伸阅读

- MCP 规范 2026-07-28：<https://modelcontextprotocol.io/specification/2026-07-28>。会话、传输和 MRTR 各条的最终依据。
- LangGraph 文档：<https://docs.langchain.com/oss/python/concepts/context>。state、checkpoint、thread 等概念的上层说明。
- OpenAI Agents SDK：<https://openai.github.io/openai-agents-python/>。Agent、Runner、handoff 和 guardrail 的接口说明。
