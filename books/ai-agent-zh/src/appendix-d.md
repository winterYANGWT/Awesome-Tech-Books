# 附录 D MCP 2026-07-28 消息速查

查消息形状，不重讲协议设计：先看传输和信封，再按方法、通知、结果字段、错误码查。字段以 2026-07-28 版 `schema.ts` 为准；和旧教程冲突时，以 schema 和本附录为准。

## D.1 传输

| 传输 | 怎么走 | 必须带 | 注意 | 正文 |
|---|---|---|---|---|
| stdio | 子进程的 stdin/stdout，一行一条 JSON-RPC 消息 | 没有 HTTP 头 | 一行不能有内嵌换行；日志写 stderr，stdout 只留协议消息 | [11.1 节](ch11.md) |
| Streamable HTTP | 每个请求 `POST` 到同一个 MCP endpoint | `MCP-Protocol-Version`、`Mcp-Method`；`tools/call`、`resources/read`、`prompts/get` 还要 `Mcp-Name` | 头和 `_meta` 不一致回 `HeaderMismatch`（-32020，HTTP 400）；没有 GET 流 | [11.1 节](ch11.md) |

- `Mcp-Name` 的值是工具名、资源 URI 或提示名。
- 没有 `initialize` 握手，没有 `Mcp-Session-Id`。需要跨调用状态时，server 发一个显式 handle，client 下次当普通工具参数传回（[10.4 节](ch10.md)）。
- `server/discover` 是 server **MUST** 实现的方法；client 可选调用它做版本选择，也可以直接在每请求 `_meta` 里内联协商（[10.4 节](ch10.md)）。

## D.2 请求和成功响应

请求 envelope：

```json
{"jsonrpc": "2.0", "id": 7, "method": "tools/call",
 "params": {"name": "get_order", "arguments": {"order_id": "A1659"},
   "_meta": {"io.modelcontextprotocol/protocolVersion": "2026-07-28",
     "io.modelcontextprotocol/clientInfo": {"name": "book-client", "version": "0.1"},
     "io.modelcontextprotocol/clientCapabilities": {}}}}
```

`params._meta` 的两个必填字段见 D.3。`id` 是字符串或整数，不能是 `null`；同一发送方未完成的请求不能重号（[10.2 节](ch10.md)）。

成功响应 envelope：

```json
{"jsonrpc": "2.0", "id": 7,
 "result": {"resultType": "complete",
   "content": [{"type": "text", "text": "订单 A1659，实付 82 元"}],
   "structuredContent": {"order_id": "A1659", "total": 82.0},
   "_meta": {"io.modelcontextprotocol/serverInfo": {"name": "shop-mcp", "version": "0.1"}}}}
```

`resultType` 每个结果都要有；`content` 是非结构化内容块，`structuredContent` 给程序解析（[10.5 节](ch10.md)）。

## D.3 `_meta`

`_meta` 放在请求的 `params` 里、通知的 `params` 里，或结果的 `result` 里。每个请求都自带版本和能力，server 不得从前一个请求推断（[10.4 节](ch10.md)）。

| 字段 | 出现位置 | 必填 | 说明 | 正文 |
|---|---|---|---|---|
| `io.modelcontextprotocol/protocolVersion` | 每个请求的 `params._meta` | 是 | 例如 `"2026-07-28"`；HTTP 上和 `MCP-Protocol-Version` 头不一致回 -32020 | [10.4 节](ch10.md) |
| `io.modelcontextprotocol/clientCapabilities` | 每个请求的 `params._meta` | 是 | 空对象表示不支持可选能力；server 不能从前一次请求推断 | [10.4 节](ch10.md) |
| `io.modelcontextprotocol/clientInfo` | 请求的 `params._meta` | 否 | `{name, version}`；只用于显示、日志、调试，不用于安全决策 | [10.4 节](ch10.md) |
| `io.modelcontextprotocol/logLevel` | 请求的 `params._meta` | 否 | 想要日志才填；不填 server 不得发 `notifications/message`。Logging 已弃用 | [11.6 节](ch11.md) |
| `progressToken` | 请求的 `params._meta` | 否 | 进度通知 `notifications/progress` 的关联 token；所有在途请求里要唯一 | [10.5 节](ch10.md) |
| `io.modelcontextprotocol/serverInfo` | 结果或响应的 `_meta` | 否 | `{name, version}`；server 自报，不用于安全决策 | [10.4 节](ch10.md) |
| `io.modelcontextprotocol/subscriptionId` | 订阅流的通知和结束结果的 `_meta` | 是（订阅流） | 等于打开该流的 `subscriptions/listen` 请求 id | [10.4 节](ch10.md) |

## D.4 方法总表

所有请求的 `params._meta` 都带 D.3 的两个必填字段，下表不再重复。每个结果都另有 `resultType`（D.6），表里只在 `tools/call` 和订阅流标出特殊值。

| 方法 | 方向 | 请求参数 | 结果字段 | 可缓存 | 正文 |
|---|---|---|---|---|---|
| `server/discover` | client → server | `_meta` | `supportedVersions[]`、`capabilities`、`instructions?`、`ttlMs`、`cacheScope` | 是 | [10.4 节](ch10.md) |
| `tools/list` | client → server | `_meta`、`cursor?` | `tools[]`、`nextCursor?`、`ttlMs`、`cacheScope` | 是 | [10.3 节](ch10.md) |
| `tools/call` | client → server | `_meta`、`name`、`arguments?`、`inputResponses?`、`requestState?` | `content[]`、`structuredContent?`、`isError?`；可能 `resultType: "input_required"` 加 `inputRequests`、`requestState` | 否 | [10.5 节](ch10.md)、[12.1 节](ch12.md) |
| `resources/list` | client → server | `_meta`、`cursor?` | `resources[]`、`nextCursor?`、`ttlMs`、`cacheScope` | 是 | [11.3 节](ch11.md) |
| `resources/templates/list` | client → server | `_meta`、`cursor?` | `resourceTemplates[]`、`nextCursor?`、`ttlMs`、`cacheScope` | 是 | [11.3 节](ch11.md) |
| `resources/read` | client → server | `_meta`、`uri` | `contents[]`、`ttlMs`、`cacheScope` | 是 | [11.3 节](ch11.md) |
| `prompts/list` | client → server | `_meta`、`cursor?` | `prompts[]`、`nextCursor?`、`ttlMs`、`cacheScope` | 是 | [11.3 节](ch11.md) |
| `prompts/get` | client → server | `_meta`、`name`、`arguments?` | `description?`、`messages[]` | 否 | [11.3 节](ch11.md) |
| `completion/complete` | client → server | `_meta`、`ref`、`argument{name,value}`、`context?` | `completion.values[]`、`total?`、`hasMore?` | 否 | [10.3 节](ch10.md) |
| `subscriptions/listen` | client → server（开流） | `_meta`、`notifications{toolsListChanged?,promptsListChanged?,resourcesListChanged?,resourceSubscriptions?}` | 长流：先 `notifications/subscriptions/acknowledged`，再通知；结束时返回 `subscriptions/listen` 的 result（`resultType` + `_meta.subscriptionId`） | 否 | [10.4 节](ch10.md) |

- 标为“是”的结果必须带 `ttlMs`（毫秒）和 `cacheScope`（`"public"` / `"private"`）。`ttlMs` 是新鲜度提示，不是轮询间隔（[10.4 节](ch10.md)）。
- MRTR（D.7）的重试结果不缓存，因为 `inputResponses` 和 `requestState` 不在缓存键里（[10.4 节](ch10.md)）。
- `completion/complete` 只在 server 的 `completions` 能力下可用（[10.3 节](ch10.md)）。

## D.5 通知

通知没有 `id`，收到也不回（[10.2 节](ch10.md)）。订阅流上的每条通知都在 `_meta` 带 `io.modelcontextprotocol/subscriptionId`，值是打开该流的 `subscriptions/listen` 请求 id（[10.4 节](ch10.md)）。

| 通知 | 方向 | 关键参数 | 何时出现 | 正文 |
|---|---|---|---|---|
| `notifications/progress` | server → client | `progressToken`、`progress`、`total?`、`message?` | 带 `progressToken` 的请求执行中；走该请求的响应流，不走订阅流 | [10.5 节](ch10.md) |
| `notifications/message` | server → client | `level`、`logger?`、`data` | 请求 `_meta.logLevel` 时，在该请求的响应流上；Logging 已弃用 | [11.6 节](ch11.md) |
| `notifications/cancelled` | client → server（stdio 上 server 也可发，用来关订阅流） | `requestId`、`reason?` | 取消在途请求或订阅 | [10.2 节](ch10.md) |
| `notifications/resources/updated` | server → client | `_meta.subscriptionId`、`uri` | `resourceSubscriptions` 里的 URI 变化 | [10.4 节](ch10.md) |
| `notifications/resources/list_changed` | server → client | `_meta.subscriptionId` | 资源列表变化；订阅时选了 `resourcesListChanged` | [10.4 节](ch10.md) |
| `notifications/tools/list_changed` | server → client | `_meta.subscriptionId` | 工具列表变化；订阅时选了 `toolsListChanged` | [10.4 节](ch10.md) |
| `notifications/prompts/list_changed` | server → client | `_meta.subscriptionId` | 提示列表变化；订阅时选了 `promptsListChanged` | [10.4 节](ch10.md) |
| `notifications/subscriptions/acknowledged` | server → client | `_meta.subscriptionId`、`notifications`（server 同意的子集） | 订阅建立后的第一条消息 | [10.4 节](ch10.md) |

## D.6 结果字段

| 字段 | 出现在 | 规则 | 正文 |
|---|---|---|---|
| `resultType` | 所有结果 | `"complete"` 或 `"input_required"`；旧 server 没这个字段时按 `"complete"` 处理 | [10.2 节](ch10.md) |
| `ttlMs` | `server/discover`、各 list、`resources/read` | 毫秒，必须 `>= 0`；0 表示立刻过期；只在 `complete` 结果上 | [10.4 节](ch10.md) |
| `cacheScope` | 同 `ttlMs` | `"public"` 可跨授权上下文共享，`"private"` 只在同一授权上下文复用 | [10.4 节](ch10.md) |
| `content` | `tools/call` | 非结构化内容块数组，文本、图片、资源链接可混在一起 | [10.5 节](ch10.md) |
| `structuredContent` | `tools/call` | 任意 JSON 值；有 `outputSchema` 时必须符合它 | [10.5 节](ch10.md) |
| `isError` | `tools/call` | 工具自身的错误放这里并设 `true`，不要用协议层错误 | [10.5 节](ch10.md) |
| `inputSchema` | `tools/list` 的每个 `Tool` | 合法 JSON Schema，根必须是 `type: "object"`；没写 `$schema` 时按 2020-12 | [11.2 节](ch11.md) |
| `outputSchema` | `tools/list` 的每个 `Tool` | 可选；有它时 `structuredContent` 必须符合它 | [11.2 节](ch11.md) |
| `annotations` | `tools/list` 的每个 `Tool` | `readOnlyHint`、`destructiveHint`、`idempotentHint`、`openWorldHint` 都是提示，不是保证 | [12.4 节](ch12.md) |

## D.7 MRTR

| 步骤 | 谁发 | 消息 | 关键字段 | 正文 |
|---|---|---|---|---|
| 1. 首次调用 | client → server | `tools/call`（也可是 `resources/read`、`prompts/get`） | `_meta`、`name`、`arguments?` | [12.1 节](ch12.md) |
| 2. 要输入 | server → client | `resultType: "input_required"` 的结果 | `inputRequests`（key → 请求对象）、`requestState?`；两者至少有一个 | [12.1 节](ch12.md) |
| 3. 重试 | client → server | 同一个调用，新 `id` | `inputResponses`（key 对应 `inputRequests`）、原样的 `requestState`、`_meta` | [12.1 节](ch12.md) |
| 4. 完成 | server → client | `resultType: "complete"` | `content[]`、`structuredContent?`、`isError?` | [12.1 节](ch12.md) |

- `inputRequests` 的值是 `sampling/createMessage`、`roots/list` 或 `elicitation/create` 请求对象。
- 只有 `tools/call`、`resources/read`、`prompts/get` 能回 `input_required`；别的请求不行（[12.1 节](ch12.md)）。
- 旧写法是 server 在连接上主动发请求；2026-07-28 改成“server 在结果里要输入，client 带答复重试”（[12.1 节](ch12.md)）。
- `requestState` 是攻击者可控的输入，server 要验完整性、TTL 和原请求标识；client 只原样回传（[12.1 节](ch12.md)）。

## D.8 错误码

| 码 | 名称 | 谁的问题 | 怎么修 | 正文 |
|---|---|---|---|---|
| -32700 | Parse error | 发送方 | JSON 没解析成功；检查是不是一行一条、有没有 `print` 混进 stdout | [10.2 节](ch10.md) |
| -32600 | Invalid Request | 发送方 | 不是合法 JSON-RPC 请求；补 `jsonrpc`、`method`，按需要补 `id`、`params` | [10.2 节](ch10.md) |
| -32601 | Method not found | 发送方 | 方法不存在或 server 没声明该能力；别发已移除的 `initialize`、`ping` | [10.2 节](ch10.md) |
| -32602 | Invalid params | 发送方 | 参数或 `_meta` 缺字段；资源找不到也用它（原 -32002） | [10.2 节](ch10.md) |
| -32603 | Internal error | 服务端 | server 内部异常；看 server 日志，重试通常还是同一个错 | [10.2 节](ch10.md) |
| -32020 | Header mismatch | 发送方或中间层 | HTTP 头和 body 对齐：`MCP-Protocol-Version` 对 `_meta`，`Mcp-Method` 对方法，`Mcp-Name` 对名称 | [11.1 节](ch11.md) |
| -32021 | Missing required client capability | 发送方 | 在 `_meta.clientCapabilities` 里声明请求要用的能力，例如 `elicitation` | [12.1 节](ch12.md) |
| -32022 | Unsupported protocol version | 双方版本不匹配 | 按 `error.data.supported` 选一个版本重试，或先调 `server/discover` | [10.4 节](ch10.md) |

`-32002`（资源不存在）已被 `-32602` 取代；旧 server 仍可能发，client 仍应接受（[10.2 节](ch10.md)）。

## D.9 旧方法对照

| 旧写法（2024–2025） | 2026-07-28 | 正文 |
|---|---|---|
| `initialize` + `notifications/initialized` | 没有握手；每个请求在 `_meta` 带版本和能力 | [10.4 节](ch10.md) |
| `Mcp-Session-Id` / 协议级会话 | 没有协议级会话；跨调用状态用显式 handle | [10.4 节](ch10.md) |
| `ping` | 移除；规范没有替代方法 | [10.2 节](ch10.md) |
| `logging/setLevel` | 每请求 `_meta.logLevel`；Logging 弃用，日志写 stderr 或用 OpenTelemetry | [11.6 节](ch11.md) |
| `resources/subscribe` / `resources/unsubscribe`（更新仍用 `notifications/resources/updated`） | `subscriptions/listen` 的 `resourceSubscriptions` | [10.4 节](ch10.md) |
| HTTP GET 端点 + SSE `Last-Event-ID` 续传 | 单端点 POST；断流后重发新请求（新 id） | [11.1 节](ch11.md) |
| `notifications/roots/list_changed` | 移除；Roots 弃用 | [10.3 节](ch10.md) |
| server 主动发 `sampling/createMessage`、`roots/list`、`elicitation/create` | MRTR：结果里回 `input_required`，client 带 `inputResponses` 重试 | [12.1 节](ch12.md) |
| `tasks/result` 阻塞等待 | `tasks/get` 和 `tasks/update` 轮询（`io.modelcontextprotocol/tasks` 扩展） | [第 12 章](ch12.md) |
| OAuth 动态客户端注册（DCR） | 预注册优先，其次 Client ID Metadata Documents（CIMD）；DCR 只做兼容 | [12.2 节](ch12.md) |
| HTTP+SSE 传输 | 弃用；改用 Streamable HTTP | [11.1 节](ch11.md) |

Roots、Sampling、Logging 还在规范里，但 2026-07-28 起进入弃用期，新实现不要采用（[10.3 节](ch10.md)）。

> **旧资料提示**：2024–2025 的 MCP 教程从 `initialize` 握手讲起；2026-07-28 没有握手，每个请求用 `_meta` 带版本和能力。`resources/subscribe` 换成 `subscriptions/listen` 的 `resourceSubscriptions`，`logging/setLevel` 换成 `_meta.logLevel`，`ping` 已移除；`tasks/result` 的阻塞等待换成 `tasks/get` 和 `tasks/update` 轮询（扩展）。

## D.10 示例

`d_1_envelope.py` 只构造消息并断言，不连接 server：

```python
{{#include code/appendix-d/d_1_envelope.py}}
```

```text
{{#include code/appendix-d/d_1_envelope.out}}
```

看三处：请求的 `_meta` 同时有 `protocolVersion` 和 `clientCapabilities`；结果是 `complete` 且 `content` 非空；整条消息里没有 `initialize`，也没有 `session` 字段。

`d_2_mrtr.py` 把一次退款的 MRTR 三段拼出来：

```python
{{#include code/appendix-d/d_2_mrtr.py}}
```

```text
{{#include code/appendix-d/d_2_mrtr.out}}
```

看三处：第一次是 `input_required`，`inputRequests` 的 key 和重试的 `inputResponses` 对得上；`requestState` 原样带回；重试的是同一个工具调用，但 JSON-RPC `id` 是新的（7 → 8），因为它是独立请求，规范要求 `id` 不同（[12.1 节](ch12.md)）。

## 延伸阅读

- MCP 规范 2026-07-28：https://modelcontextprotocol.io/specification/2026-07-28
- MCP《Key Changes》：https://modelcontextprotocol.io/specification/2026-07-28/changelog
- MCP《Base Protocol》：https://modelcontextprotocol.io/specification/2026-07-28/basic
- MCP《Tools》：https://modelcontextprotocol.io/specification/2026-07-28/server/tools
- MCP《Multi Round-Trip Requests》：https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
- schema.ts：https://github.com/modelcontextprotocol/specification/blob/main/schema/2026-07-28/schema.ts
