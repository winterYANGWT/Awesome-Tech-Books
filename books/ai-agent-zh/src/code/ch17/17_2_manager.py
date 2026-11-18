# mode: run
# timeout: 600
# expect: ask_refund
# expect: A1659
"""manager：三个专员是三个工具，专员只交回摘要，只有 manager 对顾客说话。"""
from __future__ import annotations

from harness import short
from ch17.lib.orchestration import LABELS, manager_once
from shop import Store, tickets

ROWS = {t["id"]: t for t in tickets()}

for tid in ("T05", "T06", "T24"):
    ticket = ROWS[tid]
    result = manager_once(Store(), ticket["text"])
    print(f"[{tid}] {ticket['category']}：{ticket['text']}")
    for item in result["log"]:
        print(f"  manager → ask_{item['domain']}({short(item['question'], 56)!r})")
        print(f"  {LABELS[item['domain']]} ← {short(item['answer'], 96)}")
    print(f"  manager 回答：{short(result['answer'], 150)}")
    print(f"  模型调用 {result['calls']} 次")

print()
print("发言者：manager（专员是工具，摘要是工具结果，顾客看不到）")
