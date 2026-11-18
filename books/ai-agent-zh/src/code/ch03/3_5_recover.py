# 3.5 工具报错后模型改路：退款被拒 → 提交主管
# expect: escalate
# expect: 主管
import json
import os

from openai import OpenAI

from harness import Tool
from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")

box = support_tools(Store(), include=("get_order", "issue_refund", "escalate_to_supervisor"))
# 演示“先试、报错、改路”：把描述里提前说明 500 元上限的那句去掉，工具本身不变。
refund = box.tools["issue_refund"]
box.add(Tool(refund.name, "给订单退款，钱原路退回。", refund.parameters, refund.fn, refund.risky))
messages = [
    {"role": "system", "content": SUPPORT_PROMPT},
    {"role": "user", "content": "订单 A1680 的典藏版十二卷里有三卷装订脱落，要求全额退款 680 元。"
                                "请先直接在系统里试着退，退不了再提交主管审批。"},
]

answer = None
for turn in range(1, 6):
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
        args = json.dumps(json.loads(tc.function.arguments), ensure_ascii=False)
        print(f"第 {turn} 轮 → {tc.function.name}({args})  ← {result[:90]}")
        messages.append({"role": "tool", "tool_call_id": tc.id, "content": result})

print("最终回答：")
print(answer)
