# mode: run
# timeout: 600
# expect: 15 天
# expect: 描述
"""2 个工单 × 2 种描述 × 6 次：描述写不写清“查什么、不查什么”。"""
from __future__ import annotations

from harness import run
from lib.policy_tools import policy_tools
from shop import Store, tickets
from shop.tools import SUPPORT_PROMPT

CASES = (("T14", "拆封"), ("T22", "15 天"))
REPEATS = 6


def one(ticket: dict, detailed: bool) -> dict:
    store = Store()
    messages = [{"role": "system", "content": SUPPORT_PROMPT},
                {"role": "user", "content": ticket["text"]}]
    res = run(messages, policy_tools(store, detailed), max_turns=6, verbose=False)
    queries = [str(s.arguments.get("query", "")) for s in res.steps if s.name == "search_policies"]
    return {"turns": res.turns, "tokens": res.usage.get("prompt_tokens", 0),
            "order_calls": sum(s.name == "get_order" for s in res.steps),
            "hit": bool(queries), "qlen": sum(map(len, queries)) / len(queries) if queries else 0,
            "errors": sum(s.result.startswith("错误：") for s in res.steps),
            "answer": res.answer or ""}


rows = {t["id"]: t for t in tickets()}
for tid, word in CASES:
    for label, detailed in (("空描述", False), ("完整描述", True)):
        runs = [one(rows[tid], detailed) for _ in range(REPEATS)]
        print(f"{tid} {label} | 提到“{word}” {sum(word in r['answer'] for r in runs)}/{REPEATS}"
              f" | 调 search_policies {sum(r['hit'] for r in runs)}/{REPEATS}"
              f" | query 平均 {sum(r['qlen'] for r in runs) / REPEATS:.1f} 字"
              f" | 错误 {sum(r['errors'] for r in runs)} 次"
              f" | 平均 {sum(r['turns'] for r in runs) / REPEATS:.1f} 轮")
