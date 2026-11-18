# 3.4 统计：提示里说“同时查”和“一步一步查”，第一轮各返回几个调用
# expect: 统计
# timeout: 360
import os

from openai import OpenAI

from shop import Store
from shop.tools import SUPPORT_PROMPT, support_tools

client = OpenAI()
MODEL = os.environ.get("MODEL", "deepseek-flash")
TICKET = "订单 A1659 缺了二十多页，我想退款，能退多少？"

groups = {
    "同时查": "请同时查订单和退货政策。",
    "一步一步查": "请一步一步查：先查订单，再查退货政策。",
}
stats = {name: {"calls": [], "turns": []} for name in groups}

for name, hint in groups.items():
    print(f"{name}：")
    for i in range(1, 4):
        box = support_tools(Store(), include=("get_order", "search_policies"))
        messages = [
            {"role": "system", "content": SUPPORT_PROMPT},
            {"role": "user", "content": f"第 {i} 次独立尝试。{TICKET}{hint}"},
        ]
        first = turns = 0
        first_names = "无"
        for turn in range(1, 6):
            resp = client.chat.completions.create(model=MODEL, messages=messages, tools=box.schemas())
            turns = turn
            msg = resp.choices[0].message
            calls = msg.tool_calls or []
            if turn == 1:
                first = len(calls)
                first_names = "、".join(tc.function.name for tc in calls) or "无"
            assistant = {"role": "assistant", "content": msg.content or "",
                         "tool_calls": [{"id": tc.id, "type": "function",
                                         "function": {"name": tc.function.name,
                                                      "arguments": tc.function.arguments}}
                                        for tc in calls]}
            reasoning = getattr(msg, "reasoning_content", None)
            if reasoning:
                assistant["reasoning_content"] = reasoning
            messages.append(assistant)
            if not calls:
                break
            for tc in calls:
                result = box.call(tc.function.name, tc.function.arguments)
                messages.append({"role": "tool", "tool_call_id": tc.id, "content": result})
        print(f"  第 {i} 次：第 1 轮 {first} 个调用（{first_names}），共 {turns} 轮")
        stats[name]["calls"].append(first)
        stats[name]["turns"].append(turns)

print("统计（每组 3 次）：")
for name, s in stats.items():
    parallel = sum(n > 1 for n in s["calls"])
    print(f"  {name}：第 1 轮并行 {parallel}/3 次，"
          f"平均 {sum(s['calls']) / 3:.1f} 个调用，共 {sum(s['turns'])} 轮")
