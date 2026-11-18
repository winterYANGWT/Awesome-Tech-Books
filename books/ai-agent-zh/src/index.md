# 前言

## 这本书讲什么、写给谁

这本书讲怎样用大模型做 agent：让模型在一个循环里调用工具、看结果、决定下一步，直到把一件事办完。先直接调模型 API，从一次请求讲到一个几十行的 agent 循环；再讲工具设计、MCP、上下文工程、多 agent、护栏和评估、harness 和生产；最后用 LangGraph 和 OpenAI Agents SDK 把前面手写的东西重写一遍做对照。

写给会写 Python、熟悉命令行的开发者。不需要机器学习背景，也不需要调用过大模型 API。HTTP、JSON、SQLite 不从头讲；token、上下文窗口、工具调用、JSON-RPC、OAuth 这些概念会在第一次用到时讲清楚。

## 全书路线图

九个部分从“一次模型调用”开始，到“生产中的 agent”结束。第一部分是所有后续内容的基础，之后工具、上下文、可靠性三条线可以按需要选读。

{{#fig preface/roadmap}}

| 想解决什么 | 读哪几部分 |
|---|---|
| 弄懂 agent 到底在做什么 | 第一部分，然后第三部分 |
| 用固定流程把一件事做稳 | 第一、二部分 |
| 让模型用好工具、接上现成的服务 | 第三、四部分 |
| agent 跑得久了开始犯糊涂、费用变高 | 第五部分，以及第 23 章 |
| 一个 agent 不够用 | 第五、六部分（先读第 7 章） |
| 上线前要防注入、要人工审批、要评估 | 第七部分 |
| 已经在用 LangGraph 或 Agents SDK | 第一部分、第九部分，遇到问题回查前面 |

## 贯穿全书的示例：青石书店

全书用一家虚构的网上书店做题材：40 种书、120 个顾客、约 680 张订单，退货、物流、评论都有记录，还有五份店铺政策（退货、配送、会员、发票、顾客信息）和 30 个客服工单。数据全在本地的 SQLite 里，不联网。

| 题材 | 用在哪里 |
|---|---|
| 客服工单：查物流、退款、改地址、问政策 | 前四部分的主线，第 21 章用它们做评估 |
| 退款和改地址这两个“会改数据”的工具 | 护栏、人工审批、持久执行（第 19、20、23 章） |
| “第三季度退货率为什么上升” | 第 14 章的数据探索、第 18 章的多 agent 调研 |
| 商品评论里藏着的几条恶意指令 | 第 19 章的提示注入 |

书店的数据是脚本生成的，里面埋了几个可以被查出来的原因。读到第 18 章之前，可以先自己猜一猜。

## 版本基准

| 项目 | 本书基准 | 说明 |
|---|---|---|
| Python | 3.12 | 示例用到 `asyncio`、类型注解 |
| 模型 | DeepSeek-V4.1-Flash（`deepseek-flash`） | 需要对比时用 `deepseek-v4-pro`；接口是 OpenAI 兼容的 Chat Completions |
| openai | 3.51 | 直接调 API 的示例都用它 |
| MCP | 规范 2026-07-28，Python SDK 2.3 | 这一版去掉了握手和会话，和 2025 年及以前的教程差别很大 |
| LangGraph / LangChain | 1.2 / 1.5 | 第 24–26 章 |
| OpenAI Agents SDK | 0.24 | 第 27 章 |

换成别的模型服务一般只要改环境变量；不兼容的地方（例如结构化输出的某些参数）正文会点出来，附录 A 有三种接口格式的对照。

## 怎么读示例的输出

大模型的回答每次都不一样，同样的请求连发三次，回答也可能各不相同。所以书里的每个示例都**真实运行过一次，并把那一次的全部模型调用录了下来**；书里展示的输出就是那一次的输出。

{{#fig preface/record_replay}}

这带来三点约定：

- 输出前面写的是“这次运行的输出”。正文会说“这次模型第 2 轮就调用了退款工具”，但不会把一次运行说成规律。
- 选错工具、被注入这类“有时发生”的现象，示例会自己重复多次并打印统计，正文只讲趋势。
- 每个示例旁边的 `.cassette.jsonl` 是那一次运行的全部请求和响应，想看模型到底收到了什么、回了什么，直接打开它。

## 示例代码怎么运行

每章的示例在 `code/chNN/` 下，按小节编号命名；书店在 `code/shop/`，第 3 章写出来的最小 agent 循环在 `code/harness/`。在 `code/` 目录里：

```bash
python3.12 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
export PYTHONPATH=$PWD
export OPENAI_BASE_URL=https://api.deepseek.com
export OPENAI_API_KEY=sk-...          # 你自己的 key
python ch03/3_3_loop.py
```

你运行得到的回答会和书里不同：措辞不同，有时步骤也不同。这正是 agent 要面对的问题，书里会反复讲怎样让这种不确定变得可控。

## 资料来源

本书按主题合编了下面这些公开资料，并用 2026 年 11 月的规范、文档和实际运行结果做了核对和更新；资料里已经过时的说法，正文会写明今天的做法。每篇资料讲了什么、哪些已经过时，见附录 C。

| 资料 | 作者 | 时间 |
|---|---|---|
| Building effective agents | Anthropic | 2024-12 |
| A practical guide to building agents | OpenAI | 2025-04 |
| How we built our multi-agent research system | Anthropic | 2025-06 |
| Writing effective tools for agents — with agents | Anthropic | 2025-09 |
| Effective context engineering for AI agents | Anthropic | 2025-09 |
| Equipping agents for the real world with Agent Skills | Anthropic | 2025-10 |
| Scaling Managed Agents: Decoupling the brain from the hands | Anthropic | 2026-04 |
| Model Context Protocol 文档和规范 | MCP 项目 | 规范 2026-07-28 |
| Agent Skills 规范 | agentskills.io | 2026 |
| Introduction to LangGraph（LangChain Academy）和 LangChain 文档 | LangChain | 2026 |
