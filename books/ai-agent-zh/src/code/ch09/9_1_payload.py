# 9.1 同一张 A1670，三种返回：数据库原始行、get_order、只留物流的简洁版。
# mode: run
# llm: none
# expect: 原始
# expect: get_order
# expect: 简洁
"""打印三种返回的字段数、字符数和粗略 token 估算；不调模型。"""
import json

from shop import Store
from shop.tools import support_tools

store = Store()


def rough(text: str) -> int:
    """粗略估算 token：中文按 1 字 1 个，ASCII 按 3.5 个字符 1 个。"""
    cjk = sum(1 for ch in text if ord(ch) > 0x2E80)
    return round(cjk + (len(text) - cjk) / 3.5)


def show(label: str, obj, extra: str = "") -> None:
    text = json.dumps(obj, ensure_ascii=False)
    top = len(obj) if isinstance(obj, dict) else len(obj)
    print(f"[{label}] 顶层字段 {top}{extra}  字符 {len(text)}  约 {rough(text)} token")
    print(text)


# ANCHOR: raw
# 数据库里实际存的东西：orders 一行、order_items 一行、shipments 一行（events 是 JSON 字符串）
raw = {
    **store.query("SELECT * FROM orders WHERE id = 'A1670'")[0],
    "order_items": store.query("SELECT * FROM order_items WHERE order_id = 'A1670'"),
    "shipment": store.query("SELECT * FROM shipments WHERE order_id = 'A1670'")[0],
}
show("原始", raw, "，商品 1 条，物流事件 2 条")
# ANCHOR_END: raw

# ANCHOR: get_order
tool = support_tools(store, include=("get_order",)).tools["get_order"]
order = tool.fn("A1670")                     # 直接调函数，不经过模型
show("get_order", order)
# ANCHOR_END: get_order

# ANCHOR: concise
o = store.order("A1670")
s = o["shipment"]
concise = {
    "order_id": o["id"],
    "status": o["status"],
    "courier": s["courier"],
    "delivered": s["delivered"] or "未签收",
    "latest": s["events"][-1],
    "items": [{"title": i["title"], "qty": i["qty"],
               "paid": round(i["price"] * i["qty"] * o["discount"], 2)} for i in o["items"]],
}
show("简洁", concise)
# ANCHOR_END: concise

print("说明：同一张订单，原始行带 address、内部列和 2 条事件；简洁版只留最新一条物流。")
