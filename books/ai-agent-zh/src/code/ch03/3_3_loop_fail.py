# 3.3 反例：max_turns=1 用完，没得到答案
# mode: fail
# expect: max_turns
import json
import os
import sys

from openai import OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

box = support_tools(Store(), include=("get_order", "search_policies", "issue_refund"))
messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "订单 A1680 的典藏版十二卷里有三卷装订脱落，要求全额退款。"},
]

max_turns = 1   # 只给一轮：模型还没来得及给答案就被叫停
answer = None
for turn in range(1, max_turns + 1):
    resp = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
    msg = resp.choices[0].message
    calls = msg.tool_calls or []
    assistant = {"role": "assistant", "content": msg.content or "",
                 "tool_calls": [{"id": tc.id, "type": "function",
                                 "function": {"name": tc.function.name,
                                              "arguments": tc.function.arguments}} for tc in calls]}
    reasoning = getattr(msg, "reasoning_content", None)
    if reasoning:
        assistant["reasoning_content"] = reasoning
    messages.append(assistant)
    if not calls:
        answer = msg.content
        break
    for tc in calls:
        result = box.call(tc.function.name, tc.function.arguments)
        print(f"第 {turn} 轮 → {tc.function.name}({tc.function.arguments})  ← {result[:60]}")
        messages.append({"role": "tool", "tool_call_id": tc.id, "content": result})

stop = "answer" if answer is not None else "max_turns"
print(f"stop={stop}  answer={answer!r}  turns={turn}")
if answer is None:
    print("轮数用完，没有给顾客答案", file=sys.stderr)
    sys.exit(1)
