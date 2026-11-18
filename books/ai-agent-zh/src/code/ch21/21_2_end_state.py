# 21.2 终态检查：不看回答写得多好，先看 actions 表里做了什么。
# mode: run
# llm: none
# expect: PASS
# expect: 金额 56.00
from shop import Store, tickets
from lib.eval import check_ticket

T = {t["id"]: t for t in tickets()}


def show(ticket_id: str, store: Store, result: dict) -> None:
    c = check_ticket(store, T[ticket_id], result)
    print(f"{'PASS' if c['ok'] else 'FAIL'} {ticket_id} {c['kind']} {c['reason']}")


# 1. agent 退对了：A1660 买重复，只退一本书 50 元（订单实付 56 元，运费不退）
s = Store()
s.refund("A1660", 50.0, "买重复了，只退一本")
show("T09", s, {"answer": "已为您退回 50 元。", "steps": [], "stop": "answer"})

# 2. agent 退了整单 56 元：金额多退 6 元运费，终态错
s = Store()
s.refund("A1660", 56.0, "买重复了")
show("T09", s, {"answer": "已为您全额退款 56 元。", "steps": [], "stop": "answer"})

# 3. T13 超过 500 元：agent 直接退 680，shop 拒绝，actions 为空，也没提交主管
s = Store()
fake = {"answer": "已为您退款 680 元。", "stop": "answer",
        "steps": [{"turn": 2, "name": "issue_refund",
                   "arguments": {"order_id": "A1680", "amount": 680.0}}]}
show("T13", s, fake)

# 4. T13 走对：提交主管，不直接退款
s = Store()
s.escalate("A1680", "装订脱落，全额 680 元，超过 500 元需主管批准")
show("T13", s, {"answer": "已提交主管审批，请稍等。", "steps": [], "stop": "answer"})
