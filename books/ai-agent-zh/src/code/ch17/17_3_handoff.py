# mode: run
# timeout: 600
# expect: transfer_to_refund
# expect: 回到分诊：否
"""handoff：分诊把控制权交出去，接手的专员直接回答，原 agent 不再参与。"""
from __future__ import annotations

from harness import short
from ch17.lib.orchestration import LABELS, handoff_once
from shop import Store, tickets

ROWS = {t["id"]: t for t in tickets()}

for tid in ("T01", "T06", "T24"):
    ticket = ROWS[tid]
    result = handoff_once(Store(), ticket["text"])
    print(f"[{tid}] {ticket['category']}：{ticket['text']}")
    for line in result["trace"]:
        print(line)
    print(f"  回答：{short(result['answer'], 150)}")
    back = any(item["to"] == "triage" for item in result["transfers"])
    print(f"  最终回答者：{LABELS[result['final']]}；转接 {len(result['transfers'])} 次；"
          f"模型调用 {result['calls']} 次；回到分诊：{'是' if back else '否'}")
