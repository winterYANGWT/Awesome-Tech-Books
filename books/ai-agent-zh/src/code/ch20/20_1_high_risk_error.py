# 20.1 不经过模型，直接对 A1680 调 issue_refund：看工具的错误文字，和 actions 里有没有退款。
# mode: run
# llm: none
# expect: 超过 500 元需要主管批准
from datetime import date

from shop import REFUND_LIMIT, TODAY, Store
from shop.tools import support_tools

store = Store()
box = support_tools(store, include=("issue_refund",))

print(f"退款上限 REFUND_LIMIT = {REFUND_LIMIT:.0f} 元；今天是 {TODAY}")
for order_id in ("A1680", "A1227"):
    order = store.order(order_id)
    delivered = date.fromisoformat(order["shipment"]["delivered"])
    days = (TODAY - delivered).days
    customer = store.customer(order["customer_id"])
    left = order["total"] - order["refunded"]
    print(f"\n{order_id} {customer['name']} {order['total']:.2f} 元，"
          f"签收 {delivered}（{days} 天）")
    if days <= 60:
        print("  政策：装订脱落算质量问题，60 天内可退全款；金额超过 500 要主管批准")
    else:
        print("  政策：质量问题 60 天内可以申请；这张已超过 60 天")
        print("  本章假设：政策没有覆盖超过 60 天的情况，由主管按例外处理")
    out = box.call("issue_refund", {"order_id": order_id, "amount": left, "reason": "装订脱落"})
    print(f"  issue_refund({order_id}, {left:.2f}) → {out}")

print(f"\nactions：{len(store.actions())} 条；"
      f"退款 {sum(a['kind'] == 'refund' for a in store.actions())} 条")
print("说明：两条都只拿到金额上限错误，shop 不检查 60 天期限；期限要 agent 按政策判断。")
