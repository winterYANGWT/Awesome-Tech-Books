# mode: run
# timeout: 900
# expect: T05.*issue_refund
"""单 agent 带 9 个客服工具，跑 T01–T12：看哪些工单选错工具、终态不对。"""
from __future__ import annotations

from harness import short
from ch17.lib.judge import problems, tool_line
from ch17.lib.orchestration import single_once
from shop import Store, tickets

ROWS = {t["id"]: t for t in tickets()}
TIDS = [f"T{i:02d}" for i in range(1, 13)]

ok = calls = prompt = completion = 0
logistics_refund = refund_wrong_amount = refund_no_policy = 0

for tid in TIDS:
    ticket = ROWS[tid]
    store = Store()
    res = single_once(store, ticket["text"])
    probs = problems(ticket, store, res["answer"])
    names = [step.name for step in res["steps"]]

    ok += not probs
    calls += res["calls"]
    prompt += res["usage"].get("prompt_tokens", 0)
    completion += res["usage"].get("completion_tokens", 0)
    if ticket["category"] == "物流查询" and "issue_refund" in names:
        logistics_refund += 1
    if ticket["category"] == "退货退款":
        if any(p.startswith("退款") for p in probs):
            refund_wrong_amount += 1
        if "search_policies" not in names:
            refund_no_policy += 1

    mark = "✓" if not probs else "✗ " + "；".join(probs)
    print(f"{tid} {ticket['category']} | {tool_line(res['steps'])} | {mark}")
    print(f"  回答：{short(res['answer'], 64)}")

print()
print(f"12 个工单：终态正确 {ok}/12；物流单中调了退款工具 {logistics_refund}/5；"
      f"退款单金额不对 {refund_wrong_amount}/7；退款单没查政策 {refund_no_policy}/7")
print(f"模型调用 {calls} 次，prompt {prompt / 10000:.1f} 万 token，completion {completion / 1000:.1f}k token")
