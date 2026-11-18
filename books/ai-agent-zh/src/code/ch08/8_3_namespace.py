# mode: run
# timeout: 600
# expect: 裸名
# expect: 前缀
"""2 个任务 × 2 种命名 × 5 次：裸名 search/find 和带双下划线服务前缀的名字。"""
from __future__ import annotations

from collections import Counter

from harness import run
from lib.namespace_tools import namespace_tools
from shop import Store

TASKS = [
    ("先查顾客", "顾客周子涵想买《大运河两千年》。先查她的会员等级，再查这本书的价格。", "周子涵", "大运河"),
    ("再查书", "手机号后四位 3314 的顾客想买《北方的河》。先确认顾客姓名，再查书价。", "3314", "北方"),
]
REPEATS = 5
SHOP = {"search", "shop__search_books"}
CRM = {"find", "crm__find_customers"}


def one(task: tuple[str, str, str, str], prefixed: bool) -> dict:
    _, text, customer_word, book_word = task
    store = Store()
    messages = [{"role": "system", "content": "你能用书店和 CRM 的工具。回答要给出会员等级和书价。"},
                {"role": "user", "content": text}]
    res = run(messages, namespace_tools(store, prefixed), max_turns=6, verbose=False)
    first = res.steps[0].name if res.steps else "（没调工具）"
    first_ok = first in CRM and customer_word in str(res.steps[0].arguments)
    wrong = 0
    for step in res.steps:
        blob = str(step.arguments)
        if step.name in SHOP and customer_word in blob:
            wrong += 1
        if step.name in CRM and book_word in blob:
            wrong += 1
    used_shop = any(s.name in SHOP for s in res.steps)
    used_crm = any(s.name in CRM for s in res.steps)
    return {"first": first, "first_ok": first_ok, "wrong": wrong,
            "done": used_shop and used_crm, "turns": res.turns,
            "answer": res.answer or ""}


rows = []
for task in TASKS:
    for design, prefixed in (("裸名", False), ("前缀", True)):
        runs = [one(task, prefixed) for _ in range(REPEATS)]
        rows.append((design, runs))
        first = ", ".join(f"{k}×{v}" for k, v in sorted(Counter(r["first"] for r in runs).items(),
                                                        key=lambda kv: -kv[1]))
        print(f"{task[0]} {design} | 首轮 {first} | 先查顾客 {sum(r['first_ok'] for r in runs)}/{REPEATS}"
              f" | 用错 {sum(r['wrong'] for r in runs)} 次 | 平均 {sum(r['turns'] for r in runs) / REPEATS:.1f} 轮")

print()
for design in ("裸名", "前缀"):
    runs = [r for d, rs in rows if d == design for r in rs]
    print(f"{design}组：{len(runs)} 次，先查顾客 {sum(r['first_ok'] for r in runs)}，"
          f"用错 {sum(r['wrong'] for r in runs)} 次，两件事都查到 {sum(r['done'] for r in runs)}，"
          f"平均 {sum(r['turns'] for r in runs) / len(runs):.1f} 轮")
