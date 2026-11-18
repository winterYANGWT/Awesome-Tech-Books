# mode: fail
# llm: none
# expect: 76.00
# expect: 应为 82
"""写错的退款计算：不管什么原因都只退书款，漏了 6 元运费。不调模型。"""
from __future__ import annotations

from shop import Store

store = Store()
order = store.order("A1659")
book_only = round(sum(i["price"] * i["qty"] * order["discount"] for i in order["items"]), 2)
result = store.refund("A1659", book_only, "缺页")
print(f"订单 A1659 已退款 {result['amount']:.2f} 元")

expected = round(order["total"], 2)             # 质量问题退全款：书款 76 + 运费 6
if abs(result["amount"] - expected) > 0.01:
    raise SystemExit(f"退款 {result['amount']:.2f}，应为 {expected:.2f}（含 6 元运费）")
