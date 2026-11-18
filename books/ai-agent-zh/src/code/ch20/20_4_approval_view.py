# 20.4 审批卡：只给做决定需要的字段。读 pending.json；没有就用内置的 A1680 样例。
# mode: run
# llm: none
# expect: 680.*超过 500
import json
import pathlib

from shop import REFUND_LIMIT, Store

store = Store()
pending = pathlib.Path("ch20/pending.json")
if pending.exists():
    record = json.loads(pending.read_text(encoding="utf-8"))
    refund = {**record["args"], "source": "pending.json"}
else:
    refund = {"order_id": "A1680", "amount": 680.0,
              "reason": "三卷装订脱落，按质量问题申请全款退", "source": "内置样例"}


def line(text: str = "") -> None:
    print(f"│ {text}")


# ANCHOR: refund_card
def refund_card() -> None:
    order = store.order(refund["order_id"])
    customer = store.customer(order["customer_id"])
    item = order["items"][0]
    amount = float(refund["amount"])
    print(f"┌ 退款审批卡（来源：{refund['source']}）")
    line(f"订单 {order['id']}   顾客 {customer['name']}（{customer['id']}）   "
         f"{order['status']}")
    line(f"商品 {item['title']} × {item['qty']}   签收 "
         f"{order['shipment']['delivered'] if order['shipment'] else '—'}")
    over = "  ⚠ 超过 500 元上限" if amount > REFUND_LIMIT else ""
    line(f"金额 实付 {order['total']:.2f} - 已退 {order['refunded']:.2f} "
         f"= 应退 {amount:.2f}{over}")
    line("政策 退货和退款 / 质量问题：签收后 60 天内退全款")
    line("风险 高（不可逆、金额超过 500）   模型建议：批准全额退款")
    line("操作 [批准] [拒绝]   拒绝理由：金额不对 / 不符合政策 / 需要更多信息")
    print("└")
# ANCHOR_END: refund_card


def address_card(order_id: str, new_address: str, source: str) -> None:
    order = store.order(order_id)
    customer = store.customer(order["customer_id"])
    print(f"┌ 改地址审批卡（来源：{source}）")
    line(f"订单 {order['id']}   顾客 {customer['name']}（{customer['id']}）   "
         f"{order['status']}")
    line(f"旧地址 {order['address']}")
    line(f"新地址 {new_address}")
    line("政策 配送 / 改地址：发货前可以改；已发货只能联系快递转寄或拒收")
    if order["status"] == "待发货":
        line("风险 中（可逆：发货前可以改）   模型建议：批准")
        line("操作 [批准] [拒绝]   拒绝理由：地址不对 / 需要更多信息")
    else:
        line("风险 中（不能直接改，只能转寄或拒收）   模型建议：不批准改地址")
        line("操作 [联系快递转寄] [拒收后重新下单]")
    print("└")


refund_card()
print()
address_card("A1679", "海口市幸福路 8 号", "T15")
print()
address_card("A1674", "广州市天河区体育西路 20 号", "已发货样例")
