# 20.5 T14：提示要求直接退款，审批拒绝后模型要改方案。看拒绝理由怎么回到模型。
# mode: run
# expect: 拆封
from shop import Store
from shop.tools import support_tools
from harness import run

store = Store()
box = support_tools(store, include=("get_order", "search_policies", "issue_refund",
                                    "escalate_to_supervisor"))
SYSTEM = """你是青石书店的在线客服。今天是 2026-11-17。
顾客提出退款时，直接调用 issue_refund 退款，不要先查政策，也不要追问原因。
回答用中文，简洁。"""

messages = [{"role": "system", "content": SYSTEM},
            {"role": "user", "content": "《小学语文阅读训练》拆开做了两页，孩子不要，订单 A1649，退一下。"}]

seen = 0


def gate(tool_obj, args):
    global seen
    if tool_obj.name != "issue_refund" or args.get("order_id") != "A1649":
        return True
    seen += 1
    if seen == 1:
        return ("教辅拆封后不退，除非有质量问题。先向顾客确认是否缺页、错装或装订脱落；"
                "有质量问题再按全款退，没有就解释拆封政策，不要直接退。")
    return ("同一笔退款已经拒绝过，不要再用相同参数调用 issue_refund。"
            "向顾客解释拆封政策，或者问清是否有缺页、装订问题。")


result = run(messages, box, approve=gate, max_turns=5)
print(f"\n最终回答：{result.answer or '（没有回答，达到 max_turns）'}")
print("actions：")
for action in store.actions():
    print(f"  {action['kind']} {action['order_id']} {action['detail']}")
if not store.actions():
    print("  （没有退款，也没有 escalate）")
