"""A.3 用 Anthropic Messages 格式向 DeepSeek 的 /anthropic 端点发一次工具调用。"""
# expect: tool_use
import json
import os

from anthropic import Anthropic

client = Anthropic()
MODEL = os.environ.get("MODEL", "deepseek-flash")
PARAMS = {"type": "object", "properties": {"order_id": {"type": "string"}}, "required": ["order_id"]}

message = client.messages.create(
    model=MODEL,
    max_tokens=512,
    system="你是书店客服。先查订单，再回答。",
    messages=[{"role": "user", "content": "查订单 A1659 到哪了"}],
    tools=[{"name": "get_order", "description": "按订单号查询订单状态", "input_schema": PARAMS}],
)

print("stop_reason:", message.stop_reason)
print("blocks:", [block.type for block in message.content])
for block in message.content:
    if block.type == "tool_use":
        print("tool_use:", block.name, json.dumps(block.input, ensure_ascii=False))

usage = message.usage
print("usage:", f"input_tokens={usage.input_tokens}", f"output_tokens={usage.output_tokens}",
      f"cache_read_input_tokens={usage.cache_read_input_tokens or 0}")
