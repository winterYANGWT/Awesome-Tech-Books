# mode: fail
# llm: none
# expect: T14.*不应退款
"""固定流程跑 T14：关键词“退”命中，代码错误地退了 64 元。

T14 是教辅拆封，政策规定拆封不退；固定流程没有写这个分支。
程序在最后核对工单的 expect，发现不应出现的退款，以 1 退出。
"""
import json
import re

from shop import Store, tickets

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

    result = store.refund(order["id"], order["total"], hit)
    print("退款结果：" + json.dumps(result, ensure_ascii=False))
    actions = store.actions()
    print("actions：" + json.dumps(actions, ensure_ascii=False))
    return actions


store = Store()
ticket = next(t for t in tickets() if t["id"] == "T14")
actions = handle(store, ticket)

# 工单数据里写好了正确处理应达到的终态：T14 不应有退款。
expected = ticket["expect"]["refund"]
refunds = [a for a in actions if a["kind"] == "refund"]
if expected is None and refunds:
    raise SystemExit(
        f"{ticket['id']} 不应退款：固定流程把“退”当成退款，"
        "漏掉了教辅拆封不退这个分支"
    )
print("没有发现不应出现的退款")
