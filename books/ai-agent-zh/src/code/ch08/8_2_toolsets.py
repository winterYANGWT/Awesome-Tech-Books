# mode: run
# timeout: 1200
# expect: 坏工具集
# expect: 好工具集
# expect: T01
"""6 个工单 × 2 套工具 × 4 次：统计首轮工具、轮数、输入 token 和完成率。

坏工具集是 24 个按表拆的 CRUD 包装；好工具集是 shop.tools 的 9 个任务工具。
"""
from __future__ import annotations

from collections import Counter

from lib.bad_tools import crud_tools
from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT, support_tools

TICKETS = ("T01", "T06", "T09", "T15", "T22", "T27")
REPEATS = 4


def completed(ticket: dict, store: Store, answer: str) -> bool:
    """只做计数：退款金额、地址、必须提到的词、不能提到的词。完整评估见第 21 章。"""
    exp = ticket["expect"]
    acts = store.actions()
    ok = True
    if exp.get("refund"):
        want = exp["refund"]
        ok &= any(a["kind"] == "refund" and a["order_id"] == want["order_id"]
                  and abs(a["detail"]["amount"] - want["amount"]) < 0.01 for a in acts)
    if exp.get("address_changed"):
        want = exp["address_changed"]
        ok &= any(a["kind"] == "update_address" and a["order_id"] == want["order_id"]
                  and want["contains"] in a["detail"]["new"] for a in acts)
    if exp.get("escalate"):
        ok &= any(a["kind"] == "escalate" for a in acts)
    for word in exp.get("mention", []):
        ok &= word in answer
    for word in exp.get("must_not_mention", []):
        ok &= word not in answer
    if exp.get("must_not_mention"):
        ok &= not acts
    return bool(ok)


def one(ticket: dict, design: str) -> dict:
    store = Store()
    box = crud_tools(store) if design == "坏" else support_tools(store)
    messages = [{"role": "system", "content": SUPPORT_PROMPT},
                {"role": "user", "content": ticket["text"]}]
    res = run(messages, box, max_turns=8, verbose=False)
    return {"first": res.steps[0].name if res.steps else "（没调工具）",
            "turns": res.turns,
            "tokens": res.usage.get("prompt_tokens", 0),
            "done": completed(ticket, store, res.answer or "")}


rows = {t["id"]: t for t in tickets()}
all_rows = []
for tid in TICKETS:
    for design in ("坏", "好"):
        runs = [one(rows[tid], design) for _ in range(REPEATS)]
        all_rows.append((tid, design, runs))
        first = ", ".join(f"{k}×{v}" for k, v in sorted(Counter(r["first"] for r in runs).items(),
                                                        key=lambda kv: -kv[1]))
        turns = sum(r["turns"] for r in runs) / REPEATS
        tokens = sum(r["tokens"] for r in runs) // REPEATS
        done = sum(r["done"] for r in runs)
        print(f"{tid} {design} | 首轮 {first} | 轮数 {turns:.1f} | 输入 {tokens} | 完成 {done}/{REPEATS}")

print()
for design in ("坏", "好"):
    runs = [r for _, d, rs in all_rows if d == design for r in rs]
    first = Counter(r["first"] for r in runs)
    top = "、".join(f"{k} {v}" for k, v in first.most_common(3))
    print(f"{design}工具集：{len(runs)} 次运行，完成 {sum(r['done'] for r in runs)}，"
          f"首轮最常调 {top}；平均 {sum(r['turns'] for r in runs) / len(runs):.1f} 轮、"
          f"{sum(r['tokens'] for r in runs) // len(runs)} 输入 token")
