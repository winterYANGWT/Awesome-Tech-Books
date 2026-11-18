# mode: run
# expect: issue_refund.*82
# expect: 第 3 轮.*issue_refund
"""T06 的裸 agent 循环：30 行左右，不 import harness。

只做四件事：把工具清单和对话发给模型、执行模型要的工具、把结果放回对话、再问。
没有错误恢复、日志和正式的轮数上限；第 3 章把它补成 harness.run。
"""
import json
import os

from openai import OpenAI
from shop import ShopError, Store, tickets

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")

SYSTEM = """你是“青石书店”的在线客服。今天是 2026-11-17。

- 回答之前先用工具查实际情况（订单、物流、政策），不要凭记忆或猜测回答。
- 退款、改地址之前先核对订单状态，并按政策条款判断；拿不准就提交主管，不要擅自操作。
- 只处理和本店购书有关的事；不透露其他顾客的信息。
- 回答用中文，简洁，直接告诉顾客结果和下一步。
- 工具一次只调用一个，看清结果再决定下一步。"""

TOOLS = [
    {"type": "function", "function": {
        "name": "get_order",
        "description": "查一张订单：书目、实付金额、已退金额、状态和物流。",
        "parameters": {"type": "object", "properties": {
            "order_id": {"type": "string", "description": "订单号，例如 A1659"}},
            "required": ["order_id"]}}},
    {"type": "function", "function": {
        "name": "search_policies",
        "description": "查店铺政策（退货、配送、会员等），返回最相关的 3 节原文。",
        "parameters": {"type": "object", "properties": {
            "query": {"type": "string", "description": "要查的问题，例如“质量问题 退款”"}},
            "required": ["query"]}}},
    {"type": "function", "function": {
        "name": "issue_refund",
        "description": "给订单退款。单笔超过 500 元需要主管批准；退之前先查订单和政策。",
        "parameters": {"type": "object", "properties": {
            "order_id": {"type": "string", "description": "订单号"},
            "amount": {"type": "number", "description": "退款金额（元）"},
            "reason": {"type": "string", "description": "退款原因，例如“缺页”"}},
            "required": ["order_id", "amount", "reason"]}}},
]


def call_tool(store, name, args):
    if name == "get_order":
        o = store.order(args["order_id"])
        if o is None:
            return {"error": f"订单 {args['order_id']} 不存在"}
        return {"order_id": o["id"], "status": o["status"], "total": o["total"],
                "refunded": o["refunded"], "items": [i["title"] for i in o["items"]],
                "delivered": (o["shipment"] or {}).get("delivered")}
    if name == "search_policies":
        return [{"section": h["title"], "text": h["text"]} for h in store.search_policies(args["query"])]
    if name == "issue_refund":
        try:
            return store.refund(args["order_id"], args["amount"], args["reason"])
        except ShopError as e:
            return {"error": str(e)}
    return {"error": f"没有名为 {name} 的工具"}


def fmt_args(args):
    return ", ".join(f"{k}={json.dumps(v, ensure_ascii=False)}" for k, v in args.items())


def short(text, n=160):
    text = " ".join(str(text).split())
    return text if len(text) <= n else text[: n - 1] + "…"


store = Store()
ticket = next(t for t in tickets() if t["id"] == "T06")
messages = [{"role": "system", "content": SYSTEM},
            {"role": "user", "content": ticket["text"]}]

for turn in range(1, 8):                           # 7 只是防止示例跑飞
    resp = client.chat.completions.create(model=MODEL, messages=messages, tools=TOOLS)
    msg = resp.choices[0].message
    entry = {"role": "assistant", "content": msg.content or ""}
    # 带 tools 的思考模式请求要把历史 reasoning_content 原样放回（第 3 章细讲）。
    reasoning = getattr(msg, "reasoning_content", None)
    if reasoning:
        entry["reasoning_content"] = reasoning
    if msg.tool_calls:
        entry["tool_calls"] = [{"id": tc.id, "type": "function",
                                "function": {"name": tc.function.name,
                                             "arguments": tc.function.arguments}}
                               for tc in msg.tool_calls]
    messages.append(entry)

    if not msg.tool_calls:
        print(f"stop=answer，共 {turn} 轮")
        print("回答：" + short(msg.content or ""))
        break

    for tc in msg.tool_calls:
        args = json.loads(tc.function.arguments or "{}")
        out = json.dumps(call_tool(store, tc.function.name, args), ensure_ascii=False)
        print(f"第 {turn} 轮 → {tc.function.name}({fmt_args(args)})  ← {out[:60]}")
        messages.append({"role": "tool", "tool_call_id": tc.id, "content": out})
else:
    print("stop=max_turns，共 7 轮")

print("actions=" + json.dumps(store.actions(), ensure_ascii=False))
