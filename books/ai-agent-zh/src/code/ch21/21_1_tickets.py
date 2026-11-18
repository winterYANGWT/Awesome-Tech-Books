# 21.1 评测集：30 个工单的类别和 expect。只读 shop.tickets()，不调用模型。
# mode: run
# llm: none
# expect: 30 个工单
# expect: 退货退款 9
from collections import Counter

from shop import tickets

TICKETS = tickets()


def show(expect: dict) -> str:
    parts = []
    if expect.get("refund"):
        r = expect["refund"]
        parts.append(f"refund:{r['order_id']}/{r['amount']}")
    if expect.get("address_changed"):
        a = expect["address_changed"]
        parts.append(f"addr:{a['order_id']}/*{a['contains']}*")
    if expect.get("escalate"):
        parts.append("escalate")
    for w in expect.get("mention") or []:
        parts.append(f"mention:{w}")
    for w in expect.get("must_not_mention") or []:
        parts.append(f"never:{w}")
    return " ".join(parts) or "—"


counts = Counter(t["category"] for t in TICKETS)
refunds = sum(1 for t in TICKETS if t["expect"].get("refund"))
addrs = sum(1 for t in TICKETS if t["expect"].get("address_changed"))
escalates = sum(1 for t in TICKETS if t["expect"].get("escalate"))
mentions = sum(len(t["expect"].get("mention") or []) for t in TICKETS)
nevers = sum(len(t["expect"].get("must_not_mention") or []) for t in TICKETS)

print(f"30 个工单：" + "、".join(f"{c} {n}" for c, n in counts.items()))
print(f"其中退款 {refunds} 笔、改地址 {addrs} 笔、升级 {escalates} 笔、"
      f"关键词 {mentions} 条、must_not_mention {nevers} 条")
print()
print(f"{'id':3} {'category':6} expect")
for t in TICKETS:
    print(f"{t['id']:3} {t['category']:6} {show(t['expect'])}")
