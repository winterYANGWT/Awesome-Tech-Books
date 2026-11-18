# mode: run
# expect: refund.*82
"""T06 固定流程：关键词命中后，代码查单、退款，模型只写回复。

路径是人写死的：关键词 → 查订单 → 按 order["total"] 退款 → 写回复。
"""
import json
import os
import re

from openai import OpenAI
from shop import Store, tickets

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

SYSTEM = """你是“青石书店”的在线客服。今天是 2026-11-17。

- 回答之前先用工具查实际情况（订单、物流、政策），不要凭记忆或猜测回答。
- 退款、改地址之前先核对订单状态，并按政策条款判断；拿不准就提交主管，不要擅自操作。
- 只处理和本店购书有关的事；不透露其他顾客的信息。
- 回答用中文，简洁，直接告诉顾客结果和下一步。"""

KEYWORDS = ("缺", "装订", "破损", "退")


def handle(store, ticket):
    text = ticket["text"]
    hit = next((k for k in KEYWORDS if k in text), None)
    if hit is None:
        raise SystemExit(f"{ticket['id']} 没有命中退款关键词，转人工")
    order_id = re.search(r"A\d{4}", text).group(0)

    print("工单：" + ticket["id"] + " " + text)
    print("命中关键词：" + hit)
    order = store.order(order_id)
    print(f"订单：{order['id']} {order['status']} 实付 {order['total']:.1f}")
    print(f"退款金额：order[\"total\"] = {order['total']:.1f}（不是模型写的数）")

    result = store.refund(order["id"], order["total"], hit)
    print("退款结果：" + json.dumps(result, ensure_ascii=False))
    actions = store.actions()
    print("actions：" + json.dumps(actions, ensure_ascii=False))
    return order, result


store = Store()
ticket = next(t for t in tickets() if t["id"] == "T06")
order, result = handle(store, ticket)

resp = client.chat.completions.create(
    model=MODEL,
    messages=[
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": (
            f"顾客的工单：{ticket['text']}\n"
            f"系统已经完成退款：{json.dumps(result, ensure_ascii=False)}\n"
            "请写一段给顾客的回复，说明缺页怎么处理、钱什么时候到账。"
            "不要提“系统”“工具”或内部编号。"
        )},
    ],
)
reply = " ".join((resp.choices[0].message.content or "").split())
print("模型回复：" + (reply if len(reply) <= 160 else reply[:159] + "…"))
print("模型调用：1 次")
