# mode: run
# timeout: 600
# expect: update_address
"""T15/T16 × 2 种设计 × 6 次：通用 update 的字段名自由，专用工具只有一个动作。"""
from __future__ import annotations

from collections import Counter

from lib.generic_update import update_tools
from harness import run
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT

CASES = ("T15", "T16")
REPEATS = 6


def one(ticket: dict, dedicated: bool) -> dict:
    store = Store()
    messages = [{"role": "system", "content": SUPPORT_PROMPT},
                {"role": "user", "content": ticket["text"]}]
    res = run(messages, update_tools(store, dedicated), max_turns=6, verbose=False)
    address = ticket["expect"]["address_changed"]
    acts = [a for a in store.actions() if a["kind"] == "update_address"]
    same = [a for a in acts if a["order_id"] == address["order_id"]]
    return {"fields": [str(s.arguments.get("field")) for s in res.steps if s.name == "update"],
            "errors": sum(s.result.startswith("错误：") for s in res.steps),
            "ok": any(address["contains"].replace(" ", "") in a["detail"]["new"].replace(" ", "") for a in same),
            "wrong_order": sum(a["order_id"] != address["order_id"] for a in acts),
            "turns": res.turns}


rows = {t["id"]: t for t in tickets()}
for tid in CASES:
    for label, dedicated in (("通用 update", False), ("专用 update_address", True)):
        runs = [one(rows[tid], dedicated) for _ in range(REPEATS)]
        fields = Counter(f for r in runs for f in r["fields"])
        field_text = ", ".join(f"{k}×{v}" for k, v in sorted(fields.items(), key=lambda kv: -kv[1]))
        print(f"{tid} {label} | 字段 {field_text or '（专用工具）'} | 错误 {sum(r['errors'] for r in runs)} 次"
              f" | 改对 {sum(r['ok'] for r in runs)}/{REPEATS}"
              f" | 写到别的订单 {sum(r['wrong_order'] for r in runs)} 次"
              f" | 平均 {sum(r['turns'] for r in runs) / REPEATS:.1f} 轮")
