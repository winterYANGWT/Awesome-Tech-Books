# 3.4 并行工具调用：一轮返回两个调用，回填两条 tool 消息
# expect: tool_calls
import os

from openai import OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

box = support_tools(Store(), include=("get_order", "search_policies"))
messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "订单 A1659 缺了二十多页，我想退款。请同时查一下订单和退货政策，"
                                "看看能退多少、依据是哪一条。"},
]

resp1 = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
msg = resp1.choices[0].message
calls = msg.tool_calls or []
print(f"第 1 轮 tool_calls={len(calls)} 个")
for tc in calls:
    print(f"  {tc.function.name} {tc.function.arguments}")

results = []
for tc in calls:
    results.append(box.call(tc.function.name, tc.function.arguments))
    print(f"  结果 {tc.function.name}：{results[-1][:80]}")

assistant = {"role": "assistant", "content": msg.content or "",
             "tool_calls": [{"id": tc.id, "type": "function",
                             "function": {"name": tc.function.name,
                                          "arguments": tc.function.arguments}} for tc in calls]}
reasoning = getattr(msg, "reasoning_content", None)
if reasoning:
    assistant["reasoning_content"] = reasoning
messages.append(assistant)
for tc, result in zip(calls, results):
    messages.append({"role": "tool", "tool_call_id": tc.id, "content": result})
print(f"回填 {len(calls)} 条 tool 消息")

resp2 = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
print("第 2 轮回答：")
print(resp2.choices[0].message.content)
