# 附录 A 环境配置，以及三种接口格式的对照

这一附录先给出跑通全书示例的环境和录制回放方式，再并排对照三种接口的字段。正文示例只用 Chat Completions，另外两种只用于对照。

## A.1 环境

全书示例在 Python 3.12 下运行，依赖钉在 `src/code/requirements.txt`。2026-11 的版本：

| 包 | 2026-11 版本 |
|---|---|
| `openai` | 3.51.0 |
| `langgraph` | 1.2.22 |
| `langgraph-checkpoint-sqlite` | 3.1.3 |
| `langchain` | 1.5.0 |
| `langchain-core` | 1.8.1 |
| `langchain-openai` | 1.8.0 |
| `langchain-deepseek` | 1.2.0 |
| `mcp` | 2.3.0 |
| `openai-agents` | 0.24.0 |
| `anthropic` | 2.8.0 |

在虚拟环境里安装：

```bash
python -m pip install -r src/code/requirements.txt
```

环境变量：

| 变量 | 值 | 用途 |
|---|---|---|
| `OPENAI_BASE_URL` | `https://api.deepseek.com` | Chat Completions 和 Responses 的 base URL |
| `OPENAI_API_KEY` | 你的 DeepSeek key | openai SDK、LangChain、Agents SDK 读取 |
| `MODEL` | `deepseek-flash` | 默认模型；对照时改成 `deepseek-v4-pro` |
| `ANTHROPIC_BASE_URL` | `https://api.deepseek.com/anthropic` | A.3 的 Anthropic Messages 端点 |
| `ANTHROPIC_API_KEY` | 同一把 DeepSeek key | `anthropic.Anthropic()` 读取 |

key 只放在环境变量里，不要写进代码。示例和 `.cassette.jsonl` 里都不出现 key。

`MODEL` 默认 `deepseek-flash`（DeepSeek-V4.1-Flash，1M 上下文，2026-11 时）；需要更强模型对照时用 `deepseek-v4-pro`。两个模型都支持 `low`/`high`/`max` effort，默认 `high`（[2.3 节](ch02.md)）。

在 `code/` 目录里运行示例：

```bash
export PYTHONPATH=$PWD
export OPENAI_BASE_URL=https://api.deepseek.com OPENAI_API_KEY=sk-...
python appendix-a/a_1_env.py
```

书里的 `.out` 是作者录制时那一次运行的输出，`.cassette.jsonl` 是那一次的全部请求和响应；你自己运行会得到不同但同类的结果。

```python
{{#include code/appendix-a/a_1_env.py}}
```

这次运行的输出：

```text
{{#include code/appendix-a/a_1_env.out}}
```

Python 是 3.12.13，10 个包的版本与上表一致；两个 base URL 都显示“已设置”，`MODEL` 是 `deepseek-flash`。输出里没有 key。

## A.2 三种格式对照

三个接口都把系统提示、消息和工具放进请求，但顶层字段和响应结构不同。端点相对各家的 base URL；本书示例只发 Chat Completions。

| 项目 | OpenAI Chat Completions | OpenAI Responses | Anthropic Messages | 详见 |
|---|---|---|---|---|
| 端点 | `POST /chat/completions` | `POST /responses` | `POST /v1/messages` | [2.5 节](ch02.md) |
| 顶层输入 | `model`、`messages`、`tools`、`tool_choice`、`response_format`、`stream` | `model`、`input`、`instructions`、`tools`、`text`、`stream`、`previous_response_id`、`max_output_tokens` | `model`、`max_tokens`（必填）、`system`、`messages`、`tools`、`stream` | [2.1 节](ch02.md)、[2.5 节](ch02.md) |
| 消息角色 | `system`/`user`/`assistant`/`tool` | `input` 里是 item 列表（`message`、`function_call`、`function_call_output` 等） | 只有 `user`/`assistant`；`system` 是顶层字段；工具结果放在 `user` 的 `tool_result` block | [2.1 节](ch02.md) |
| 工具定义 | `{"type":"function","function":{"name","description","parameters"}}` | `{"type":"function","name","description","parameters","strict"}` | `{"name","description","input_schema"}` | [3.1 节](ch03.md) |
| 模型发起工具调用 | `choices[0].message.tool_calls[]`（`id`、`function.name`、`function.arguments`） | `output[]` 里的 `function_call`（`call_id`、`name`、`arguments`） | `content[]` 里的 `tool_use` block（`id`、`name`、`input`） | [3.2 节](ch03.md) |
| 工具结果回填 | `{"role":"tool","tool_call_id":...,"content":...}` | `{"type":"function_call_output","call_id":...,"output":...}` | `{"role":"user","content":[{"type":"tool_result","tool_use_id":...,"content":...}]}` | [3.2 节](ch03.md) |
| 结构化输出 | `response_format={"type":"json_object"}` 或 `{"type":"json_schema",...}` | `text.format` 里指定 JSON Schema | 无 `response_format`；用 `output_config.format`（`json_schema`）或工具/提示约束 | [4.2 节](ch04.md)、[4.5 节](ch04.md) |
| 流式 | `stream=True`，chunk 里 `delta.content`/`delta.tool_calls` | `stream=True`，事件类型 `response.output_text.delta` 等 | `stream=True`，事件 `content_block_delta` 等 | [2.4 节](ch02.md) |
| usage 字段 | `prompt_tokens`、`completion_tokens`、`total_tokens`（DeepSeek 另有 `prompt_cache_hit_tokens`/`prompt_cache_miss_tokens`） | `input_tokens`、`output_tokens`、`input_tokens_details.cached_tokens` | `input_tokens`、`output_tokens`、`cache_read_input_tokens` | [2.2 节](ch02.md)、[23.2 节](ch23.md) |
| DeepSeek 支持 | 是，`https://api.deepseek.com` | 是，2026-11 实测 `POST /responses` 可用；本书示例不用 | 是，`/anthropic` 兼容端点 | [2.5 节](ch02.md) |

同一个“查订单 A1659”任务的三条请求体。A.2 只打印，不发送；A.3 才真的发一次：

```python
{{#include code/appendix-a/a_2_request_shapes.py}}
```

这次运行的输出：

```text
{{#include code/appendix-a/a_2_request_shapes.out}}
```

差别集中在四处：

- Chat 的系统提示在 `messages[0]`，Responses 在 `instructions`，Anthropic 在顶层 `system`。
- 工具定义分别包在 `function`、`type` 加 `strict`、`input_schema` 里。
- 模型发起调用时，Chat 用 `tool_calls`，Responses 用 `function_call`，Anthropic 用 `tool_use`。
- 工具结果分别放在 `role:"tool"` 消息、`function_call_output` item、`user` 消息的 `tool_result` block 里。

## A.3 同一个工具调用的三步

把 A.2 的三个请求体按“请求、响应、回填”拆开，字段对应如下。表里的字段名来自 A.2 的请求体和 A.3 的响应；`call_1` 和 `toolu_1` 是示例里写死的 id。

| 步骤 | OpenAI Chat Completions | OpenAI Responses | Anthropic Messages | 详见 |
|---|---|---|---|---|
| 请求：系统提示 | `messages[0].role="system"` | `instructions` | 顶层 `system` | [2.1 节](ch02.md) |
| 请求：用户消息 | `messages[]` 里的 `role:"user"` | `input[]` 里的 `message` item | `messages[]` 里的 `role:"user"` | [2.1 节](ch02.md) |
| 请求：工具定义 | `{"type":"function","function":{"name","description","parameters"}}` | `{"type":"function","name","description","parameters","strict"}` | `{"name","description","input_schema"}` | [3.1 节](ch03.md) |
| 响应：模型发起调用 | `tool_calls[]` 的 `id`、`function.name`、`function.arguments`（JSON 字符串） | `output[]` 的 `function_call`：`call_id`、`name`、`arguments`（JSON 字符串） | `content[]` 的 `tool_use`：`id`、`name`、`input`（对象） | [3.2 节](ch03.md) |
| 回填：工具结果 | 新消息 `{"role":"tool","tool_call_id":"call_1","content":...}` | 新 item `{"type":"function_call_output","call_id":"call_1","output":...}` | `user` 消息里的 `{"type":"tool_result","tool_use_id":"toolu_1","content":...}` | [3.2 节](ch03.md) |
| 响应：usage | `prompt_tokens`/`completion_tokens`/`total_tokens` | `input_tokens`/`output_tokens`/`input_tokens_details.cached_tokens` | `input_tokens`/`output_tokens`/`cache_read_input_tokens` | [2.2 节](ch02.md) |

A.3 用 Anthropic Messages 发一次请求，带 `get_order` 工具：

```python
{{#include code/appendix-a/a_3_anthropic_messages.py}}
```

这次运行的输出：

```text
{{#include code/appendix-a/a_3_anthropic_messages.out}}
```

`stop_reason="tool_use"` 表示模型停在工具调用上；`content` 里这次是 `thinking` 和 `tool_use` 两个 block。`tool_use.input` 已经是对象，不需要再 `json.loads`。`usage` 把输入拆成 `input_tokens=182` 和 `cache_read_input_tokens=128`，输出是 `output_tokens=197`。

这里没有指定 `tool_choice`：DeepSeek 的思考模式不接受强制工具选择，Chat 和 Anthropic 端点都会返回 400（[3.1 节](ch03.md)）。

## A.4 常见兼容坑

| 坑 | 现象 | 处理 | 详见 |
|---|---|---|---|
| Chat 的 `json_schema` 不支持 | Chat 的 `response_format={"type":"json_schema"}` 在 DeepSeek 上返回 400；Agents SDK 在 `chat_completions` 模式下的 `output_type` 也走这条路 | 改走 Responses 接口的 `text.format`（支持 `json_schema`），或用 `json_object`、强制工具 | [4.5 节](ch04.md)、[27.3 节](ch27.md) |
| `max_tokens` 必填 | Anthropic 的 `max_tokens` 必填；Responses 用 `max_output_tokens`；Chat 可省，DeepSeek 不设时按 8K/64K/128K | 跨格式迁移时先补这个字段 | [2.6 节](ch02.md) |
| `system` 位置 | Chat 在 `messages[0]`；Responses 在 `instructions`；Anthropic 在顶层 `system`，不能放进 `messages` | 不要只改字段名，消息数组的结构也要改 | [2.1 节](ch02.md) |
| 工具结果角色 | Chat 用 `role:"tool"`；Responses 用 `function_call_output` item；Anthropic 用 `user` 消息里的 `tool_result` block | id 要对上：`tool_call_id`、`call_id`、`tool_use_id` | [3.2 节](ch03.md) |
| usage 字段名 | Chat 是 `prompt_tokens`/`completion_tokens`；Responses 是 `input_tokens`/`output_tokens`；Anthropic 也是 `input_tokens`/`output_tokens`，缓存另有 `cache_read_input_tokens` | 统计和计费代码不要直接复用字段名 | [2.2 节](ch02.md) |

> **旧资料提示**：早期 Chat Completions 用 `functions` 和 `function_call` 字段。现在写成 `tools` 和 `tool_calls`，见 [3.1 节](ch03.md)。

> **旧资料提示**：把 Responses 当成 Chat Completions 的“新名字”会踩坑。两者顶层字段和输出结构不同，`input`/`output` 不能直接换成 `messages`/`choices`，见 [2.5 节](ch02.md)。

> **旧资料提示**：Anthropic Messages 的 `max_tokens` 曾经可以省略。现在必填，见 [2.5 节](ch02.md)。

## 延伸阅读

- OpenAI《Chat Completions API》：<https://platform.openai.com/docs/api-reference/chat>，`messages`、`tools`、`tool_calls` 的字段定义。
- OpenAI《Responses API》：<https://platform.openai.com/docs/api-reference/responses>，`input`、`output`、`text.format` 和流式事件。
- Anthropic《Messages API》：<https://platform.claude.com/docs/en/api/messages>，`system`、`max_tokens`、`tool_use` 和 `tool_result`。
- DeepSeek《API 文档》：<https://api-docs.deepseek.com/>，OpenAI 兼容接口、`/anthropic` 端点、模型和 `usage`。
- LangChain《ChatOpenAI 集成》：<https://docs.langchain.com/oss/python/integrations/chat/openai>，`ChatOpenAI` 的调用和工具绑定。
