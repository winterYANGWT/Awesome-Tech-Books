# 3.1 用 tool_choice 强制模型调用指定的函数
# expect: tool_calls
# expect: get_order
import os

from openai import OpenAI

from shop import Store
from shop.tools import support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

box = support_tools(Store(), include=("get_order",))
resp = client.chat.completions.create(
    model=MODEL,
    messages=[{"role": "user", "content": "查一下订单 A1659 现在到哪个环节了。"}],
    tools=box.schemas(),
    tool_choice={"type": "function", "function": {"name": "get_order"}},
    # DeepSeek 的思考模式不支持指定函数，会返回 400；这个示例关掉思考。
    extra_body={"thinking": {"type": "disabled"}},
)
msg = resp.choices[0].message
print(f"finish_reason={resp.choices[0].finish_reason}")
print(f"content={msg.content!r}")
print(f"tool_calls={len(msg.tool_calls)} 个")
for tc in msg.tool_calls:
    print(f"  {tc.function.name} {tc.function.arguments}")
