"""2026-07-28 的 tools/call 请求和 complete 结果：_meta、resultType、content 都在。"""
# mode: run
# llm: none
# expect: 校验通过
# expect: 没有 initialize
# expect: 没有 session

import json

META = {
    "io.modelcontextprotocol/protocolVersion": "2026-07-28",
    "io.modelcontextprotocol/clientInfo": {"name": "book-client", "version": "0.1"},
    "io.modelcontextprotocol/clientCapabilities": {},
}

request = {
    "jsonrpc": "2.0", "id": 7, "method": "tools/call",
    "params": {
        "name": "get_order",
        "arguments": {"order_id": "A1659"},
        "_meta": META,
    },
}

result = {
    "jsonrpc": "2.0", "id": 7,
    "result": {
        "resultType": "complete",
        "content": [{"type": "text", "text": "订单 A1659，实付 82 元"}],
        "structuredContent": {"order_id": "A1659", "total": 82.0},
        "_meta": {"io.modelcontextprotocol/serverInfo": {"name": "shop-mcp", "version": "0.1"}},
    },
}


def compact(message):
    return json.dumps(message, ensure_ascii=False, separators=(",", ":"))


meta = request["params"]["_meta"]
assert "io.modelcontextprotocol/protocolVersion" in meta
assert "io.modelcontextprotocol/clientCapabilities" in meta
assert result["id"] == request["id"]
assert result["result"]["resultType"] == "complete"
assert result["result"]["content"][0]["type"] == "text"
assert "initialize" not in compact(request)
assert "session" not in compact(request).lower()

print("请求 tools/call:")
print(compact(request))
print()
print("成功结果:")
print(compact(result))
print()
print("校验通过：_meta 两个必填字段在，resultType=complete，content 非空。")
print("没有 initialize 握手，没有 session 字段。")
