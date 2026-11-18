# 3.3 手写 agent 循环：调用模型 → 执行工具 → 回填 → 下一轮
# expect: issue_refund.*82
# expect: stop=answer
import json
import os

from openai import OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

box = support_tools(Store(), include=("get_order", "search_policies", "issue_refund"))
messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？"},
]

max_turns = 6
answer = None
prompt_tokens = completion_tokens = 0
for turn in range(1, max_turns + 1):
    resp = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
    prompt_tokens += resp.usage.prompt_tokens
    completion_tokens += resp.usage.completion_tokens
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
        args = json.dumps(json.loads(tc.function.arguments), ensure_ascii=False)
        print(f"第 {turn} 轮 → {tc.function.name}({args})  ← {result[:60]}")
        messages.append({"role": "tool", "tool_call_id": tc.id, "content": result})

stop = "answer" if answer is not None else "max_turns"
print(f"stop={stop}  turns={turn}  usage={prompt_tokens}+{completion_tokens} token")
print(answer)
