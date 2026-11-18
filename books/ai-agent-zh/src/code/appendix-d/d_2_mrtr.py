"""MRTR：server 回 input_required，client 带 inputResponses 和原 requestState 重试。"""
# mode: run
# llm: none
# expect: 校验通过
# expect: 原样带回
# expect: id 7 → 8

import json

META = {
    "io.modelcontextprotocol/protocolVersion": "2026-07-28",
    "io.modelcontextprotocol/clientInfo": {"name": "book-client", "version": "0.1"},
    "io.modelcontextprotocol/clientCapabilities": {"elicitation": {}},
}
REQUEST_STATE = "st_7Qm2Xk9pR4vN8sT1cL6hF3dW0aZ5bY"

first_request = {
    "jsonrpc": "2.0", "id": 7, "method": "tools/call",
    "params": {"name": "refund", "arguments": {"order_id": "A1680", "amount": 680}, "_meta": META},
}

input_required = {
    "jsonrpc": "2.0", "id": 7,
    "result": {
        "resultType": "input_required",
        "inputRequests": {
            "__main__:approval": {
                "method": "elicitation/create",
                "params": {
                    "mode": "form",
                    "message": "批准 A1680 退款 680 元？",
                    "requestedSchema": {
                        "type": "object",
                        "properties": {"approved": {"type": "boolean"}, "note": {"type": "string"}},
                        "required": ["approved"],
                    },
                },
            },
        },
        "requestState": REQUEST_STATE,
        "_meta": {"io.modelcontextprotocol/serverInfo": {"name": "shop-mcp", "version": "0.1"}},
    },
}

retry_request = {
    "jsonrpc": "2.0", "id": 8, "method": "tools/call",
    "params": {
        "name": "refund",
        "arguments": {"order_id": "A1680", "amount": 680},
        "inputResponses": {
            "__main__:approval": {
                "action": "accept",
                "content": {"approved": True, "note": "M01"},
            },
        },
        "requestState": input_required["result"]["requestState"],
        "_meta": META,
    },
}

complete = {
    "jsonrpc": "2.0", "id": 8,
    "result": {
        "resultType": "complete",
        "content": [{"type": "text", "text": "已退款 A1680，680 元，批准人 M01"}],
        "structuredContent": {"order_id": "A1680", "refunded": 680.0, "approved_by": "M01"},
        "_meta": {"io.modelcontextprotocol/serverInfo": {"name": "shop-mcp", "version": "0.1"}},
    },
}


def compact(message):
    return json.dumps(message, ensure_ascii=False, separators=(",", ":"))


first = input_required["result"]
assert first["resultType"] == "input_required"
assert set(first["inputRequests"]) == set(retry_request["params"]["inputResponses"])
assert retry_request["params"]["requestState"] == first["requestState"]
assert retry_request["id"] != first_request["id"]
assert retry_request["params"]["name"] == first_request["params"]["name"]
assert complete["result"]["resultType"] == "complete"

print("1) 服务端要输入（input_required）:")
print(compact(input_required))
print()
print("2) 客户端重试同一个调用（新 id）:")
print(compact(retry_request))
print()
print("3) 服务端完成:")
print(compact(complete))
print()
print("校验通过：inputResponses 的 key 对得上，requestState 原样带回，id 7 → 8。")
