# mode: run
# timeout: 1200
# expect: 单 agent
"""8 个工单 × 3 种配置：调用次数、prompt token、终态正确数。"""
from __future__ import annotations

from collections import Counter

from ch17.lib.judge import problems
from ch17.lib.orchestration import LABELS, handoff_once, manager_once, single_once
from shop import Store, tickets

ROWS = {t["id"]: t for t in tickets()}
TIDS = ("T01", "T05", "T06", "T09", "T12", "T15", "T19", "T24")
CONFIGS = ("单 agent", "manager", "handoff")


def run_one(config: str, ticket: dict) -> dict:
    store = Store()
    if config == "单 agent":
        result = single_once(store, ticket["text"])
        final = "单 agent"
    elif config == "manager":
        result = manager_once(store, ticket["text"])
        final = "manager"
    else:
        result = handoff_once(store, ticket["text"])
        final = LABELS[result["final"]]
    return {"ok": not problems(ticket, store, result["answer"]), "calls": result["calls"],
            "prompt": result["usage"].get("prompt_tokens", 0), "final": final}


runs = {config: {tid: run_one(config, ROWS[tid]) for tid in TIDS} for config in CONFIGS}

print("工单  " + "  ".join(CONFIGS))
for tid in TIDS:
    cells = []
    for config in CONFIGS:
        r = runs[config][tid]
        cells.append(f"{'✓' if r['ok'] else '✗'}{r['calls']}")
    print(f"{tid}  " + "  ".join(cells))

print()
for config in CONFIGS:
    items = list(runs[config].values())
    print(f"{config}：{len(items)} 次运行，模型调用 {sum(r['calls'] for r in items)}，"
          f"平均 prompt {sum(r['prompt'] for r in items) / len(items):.0f}，"
          f"终态正确 {sum(r['ok'] for r in items)}/{len(items)}")

finals = Counter(runs["handoff"][tid]["final"] for tid in TIDS)
print("handoff 最终回答者：" + "、".join(f"{name} {n}" for name, n in finals.items()))
