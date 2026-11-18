"""A.2 同一个任务在三种接口里的请求体：查订单 A1659。"""
# llm: none
# expect: function_call_output
import json

MODEL = "deepseek-flash"
SYSTEM = "你是书店客服，先查订单。"
QUESTION = "查订单 A1659"
ORDER = json.dumps({"id": "A1659", "status": "已签收"}, ensure_ascii=False)
ARGS = json.dumps({"order_id": "A1659"}, ensure_ascii=False)
PARAMS = {"type": "object", "properties": {"order_id": {"type": "string"}}, "required": ["order_id"]}

chat = {
    "model": MODEL,
    "messages": [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": QUESTION},
        {"role": "assistant", "content": None,
         "tool_calls": [{"id": "call_1", "type": "function",
                         "function": {"name": "get_order", "arguments": ARGS}}]},
        {"role": "tool", "tool_call_id": "call_1", "content": ORDER},
    ],
    "tools": [{"type": "function",
               "function": {"name": "get_order", "description": "查订单",
                            "parameters": PARAMS}}],
}

responses = {
    "model": MODEL,
    "max_output_tokens": 512,
    "instructions": SYSTEM,
    "input": [
        {"type": "message", "role": "user", "content": QUESTION},
        {"type": "function_call", "call_id": "call_1", "name": "get_order", "arguments": ARGS},
        {"type": "function_call_output", "call_id": "call_1", "output": ORDER},
    ],
    "tools": [{"type": "function", "name": "get_order", "description": "查订单",
               "parameters": PARAMS, "strict": True}],
}

anthropic = {
    "model": MODEL,
    "max_tokens": 512,
    "system": SYSTEM,
    "messages": [
        {"role": "user", "content": QUESTION},
        {"role": "assistant", "content": [
            {"type": "tool_use", "id": "toolu_1", "name": "get_order", "input": {"order_id": "A1659"}}]},
        {"role": "user", "content": [
            {"type": "tool_result", "tool_use_id": "toolu_1", "content": ORDER}]},
    ],
    "tools": [{"name": "get_order", "description": "查订单", "input_schema": PARAMS}],
}


def show(label: str, endpoint: str, body: dict) -> None:
    """按 JSON 打印；短对象留在一行，长对象展开，60 行以内能看清字段名。"""
    print(f"{label}  POST {endpoint}")
    print(fmt(body))
    print()


def fmt(value, level: int = 0) -> str:
    compact = json.dumps(value, ensure_ascii=False)
    if not isinstance(value, (dict, list)) or (level > 0 and len(compact) <= 180):
        return compact
    pad, inner = "  " * level, "  " * (level + 1)
    if isinstance(value, dict):
        rows = [f'{inner}{json.dumps(k, ensure_ascii=False)}: {fmt(v, level + 1)}' for k, v in value.items()]
        return "{\n" + ",\n".join(rows) + "\n" + pad + "}"
    rows = [f"{inner}{fmt(v, level + 1)}" for v in value]
    return "[\n" + ",\n".join(rows) + "\n" + pad + "]"


print("同一个任务：查订单 A1659；三条请求都带 get_order，工具结果都一样。\n")
show("OpenAI Chat Completions", "/chat/completions", chat)
show("OpenAI Responses", "/responses", responses)
show("Anthropic Messages", "/v1/messages", anthropic)
