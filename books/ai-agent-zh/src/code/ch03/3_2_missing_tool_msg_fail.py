# 3.2 反例：只回填 assistant 的 tool_calls，不跟 role:tool
# mode: fail
# expect: tool
import os
import sys

from openai import BadRequestError, OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

box = support_tools(Store(), include=("get_order",))
messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "我的订单 A1670 到哪了？什么时候能到？"},
]

resp1 = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
msg = resp1.choices[0].message
tc = msg.tool_calls[0]
print(f"第 1 次响应：{tc.function.name} {tc.function.arguments}")
assistant = {"role": "assistant", "content": msg.content or "",
             "tool_calls": [{"id": tc.id, "type": "function",
                             "function": {"name": tc.function.name,
                                          "arguments": tc.function.arguments}}]}
reasoning = getattr(msg, "reasoning_content", None)
if reasoning:
    assistant["reasoning_content"] = reasoning
messages.append(assistant)
# 故意漏掉这一条：messages.append({"role": "tool", ...})
try:
    client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
except BadRequestError as e:
    print(f"API 原文：{e}", file=sys.stderr)
    sys.exit(1)
print("没有报错：模型居然接受了缺 tool 结果的对话", file=sys.stderr)
sys.exit(2)
