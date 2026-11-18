# 3.2 一次工具调用：回填 assistant 的 tool_calls 和一条 role:tool
# expect: get_order.*A1670
# expect: 迅达
import os

from openai import OpenAI

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
print(f"第 1 次响应：finish_reason={resp1.choices[0].finish_reason}")
print(f"  id={tc.id}")
print(f"  name={tc.function.name}  arguments={tc.function.arguments}")
print(f"  arguments 的类型是 {type(tc.function.arguments).__name__}，不是 dict")

result = box.call(tc.function.name, tc.function.arguments)
print(f"工具结果：{result}")

# 先把模型这一轮的话原样放回对话，再把工具结果作为一条 tool 消息放回去。
assistant = {"role": "assistant", "content": msg.content or "",
             "tool_calls": [{"id": tc.id, "type": "function",
                             "function": {"name": tc.function.name,
                                          "arguments": tc.function.arguments}}]}
reasoning = getattr(msg, "reasoning_content", None)
if reasoning:
    assistant["reasoning_content"] = reasoning
messages.append(assistant)
messages.append({"role": "tool", "tool_call_id": tc.id, "content": result})

resp2 = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
print("第 2 次响应：")
print(resp2.choices[0].message.content)
