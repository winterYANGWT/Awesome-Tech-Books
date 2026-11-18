# mode: fail
# timeout: 600
# expect: HandoffLoop
# expect: 转接被拒绝
"""乒乓交接：退款专员缺政策工具，转回分诊；分诊又转回退款；超过上限抛 HandoffLoop。"""
from __future__ import annotations

import sys

from ch17.lib.orchestration import HandoffLoop, handoff_once
from shop import Store, tickets

ROWS = {t["id"]: t for t in tickets()}
ticket = ROWS["T06"]
print(f"[T06] {ticket['text']}")
print("退款专员的工具：get_order、issue_refund、escalate_to_supervisor、transfer_to_triage（没有 search_policies）")

try:
    result = handoff_once(Store(), ticket["text"], max_handoffs=3, allow_back=True)
except HandoffLoop as exc:
    for line in exc.trace:
        print(line)
    print(f"HandoffLoop: {exc}", file=sys.stderr)
    sys.exit(1)

for line in result["trace"]:
    print(line)
print("这次没有触发 HandoffLoop，最后的回答者是", result["final"])
