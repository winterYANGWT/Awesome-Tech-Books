# mode: run
# timeout: 600
# expect: 发言者
# expect: 分诊、退款专员
"""同一张 T06 工单分别走 manager 和 handoff：数谁对顾客说过话、最后谁回答。"""
from __future__ import annotations

from harness import short
from ch17.lib.orchestration import LABELS, handoff_once, manager_once
from shop import Store, tickets

question = {t["id"]: t for t in tickets()}["T06"]["text"]
print(f"T06：{question}")

for name in ("manager", "handoff"):
    if name == "manager":
        result = manager_once(Store(), question)
        final = "manager"
    else:
        result = handoff_once(Store(), question)
        final = LABELS[result["final"]]
    names = ["manager" if s == "manager" else LABELS[s] for s in result["speakers"]]
    print(f"{name}：发言者 {'、'.join(names)}（{len(names)} 个）；最终回答者 {final}；"
          f"模型调用 {result['calls']} 次")
    print(f"  回答：{short(result['answer'], 150)}")
