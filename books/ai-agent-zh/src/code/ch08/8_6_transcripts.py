# mode: run
# llm: none
# expect: list_all_orders
# expect: 错误
"""读 8_2 的录制，按一次运行聚合：工具调用、错误、每轮输入 token。"""
from __future__ import annotations

import json
from collections import Counter, defaultdict

from shop import tickets

PATH = "ch08/8_2_toolsets.cassette.jsonl"
TICKET_ID = {t["text"]: t["id"] for t in tickets()}
lines = [json.loads(line) for line in open(PATH, encoding="utf-8") if line.strip()]

runs = []
for entry in lines:
    msgs = entry["request"]["messages"]
    if len(msgs) == 2:
        runs.append({"ticket": TICKET_ID[msgs[1]["content"]],
                     "design": "坏" if len(entry["request"]["tools"]) == 24 else "好",
                     "calls": [], "tool_msgs": {}})
    run = runs[-1]
    run["calls"].append(entry)
    for m in entry["request"]["messages"]:
        if m.get("role") == "tool":
            run["tool_msgs"][m.get("tool_call_id")] = m.get("content", "")

for run in runs:
    names = []
    for entry in run["calls"]:
        msg = json.loads(entry["body"])["choices"][0]["message"]
        for tc in msg.get("tool_calls") or []:
            names.append(tc["function"]["name"])
    run["names"] = names
    run["errors"] = sum(str(v).startswith("错误：") for v in run["tool_msgs"].values())

for design in ("坏", "好"):
    group = [r for r in runs if r["design"] == design]
    calls = Counter(n for r in group for n in r["names"])
    errors = sum(r["errors"] for r in group)
    per_ticket = Counter(r["ticket"] for r in group if r["errors"])
    print(f"\n{design}工具集：{len(group)} 次运行，{sum(len(r['calls']) for r in group)} 次模型调用")
    print("  工具调用：" + "、".join(f"{k} {v}" for k, v in calls.most_common(6)))
    detail = "、".join(f"{k} {v}" for k, v in sorted(per_ticket.items()))
    print(f"  “错误：” {errors} 条" + (f"；{detail}" if detail else ""))
    by_call = defaultdict(list)
    for r in group:
        for i, e in enumerate(r["calls"]):
            by_call[i].append(e["usage"].get("prompt_tokens", 0))
    trend = "，".join(f"第 {i + 1} 轮 {sum(v) // len(v)}" for i, v in sorted(by_call.items())[:4])
    print(f"  输入 token（每次调用累计）：{trend}")
