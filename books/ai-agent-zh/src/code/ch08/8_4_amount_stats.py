# mode: run
# timeout: 900
# expect: 金额错误
"""T06/T09 × 2 种设计 × 8 次：模型自己算金额，还是工具按 kind 算。"""
from __future__ import annotations

from collections import Counter

from lib.amount_tools import amount_tools
from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT

CASES = (("T06", 82.0), ("T09", 50.0))
REPEATS = 8
SYSTEM = SUPPORT_PROMPT + "\n- 本组工单的顾客已经确认退款，直接调用 issue_refund 办理，不要再询问或只列选项。"


def one(ticket: dict, design: str) -> dict:
    store = Store()
    messages = [{"role": "system", "content": SYSTEM},
                {"role": "user", "content": ticket["text"]}]
    res = run(messages, amount_tools(store, design), max_turns=6, verbose=False)
    refunds = [a["detail"]["amount"] for a in store.actions() if a["kind"] == "refund"]
    return {"amounts": refunds, "turns": res.turns, "answer": res.answer or ""}


rows = {t["id"]: t for t in tickets()}
totals = {}
for tid, expected in CASES:
    for design in ("自算金额", "工具计算"):
        runs = [one(rows[tid], design) for _ in range(REPEATS)]
        amounts = Counter(f"{a:.2f}" for r in runs for a in r["amounts"])
        wrong = sum(1 for r in runs if len(r["amounts"]) != 1 or abs(r["amounts"][0] - expected) > 0.01)
        under = sum(1 for r in runs if not r["amounts"] or r["amounts"][0] < expected - 0.01)
        over = sum(1 for r in runs if r["amounts"] and r["amounts"][0] > expected + 0.01)
        dist = ", ".join(f"{k}×{v}" for k, v in sorted(amounts.items())) or "未退款"
        print(f"{tid} {design} | 退款 {dist} | 金额错误 {wrong}/{REPEATS} | 少退 {under}、多退 {over}")
        key = design
        p = totals.setdefault(key, {"wrong": 0, "under": 0, "over": 0, "n": 0})
        p["wrong"] += wrong
        p["under"] += under
        p["over"] += over
        p["n"] += REPEATS

print()
for design, p in totals.items():
    print(f"{design}：{p['n']} 次，金额错误 {p['wrong']}，其中少退 {p['under']}、多退 {p['over']}")
