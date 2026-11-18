# 20.2 三张批准单：批准、拒绝、超时。逐张走 approve 回调，再决定要不要调用工具。
# mode: run
# llm: none
# expect: 已退款
# expect: 审批超时
from ch20._lib.approvals import (NOW, approve_callback, approval_tools, decide, get,
                            new_request)
from shop import Store

store = Store()
box = approval_tools(store)

# ANCHOR: records
# 三张单：A1680 批准；A1649 拒绝；A1661 拖到过期。
new_request(store, "A1680", 680.0, "装订脱落，按质量问题退全款",
            request_id="R-1001", conversation_id="T13")
decide(store, "R-1001", "approved", "主管", "同意按质量问题全款退", at=NOW)
new_request(store, "A1649", 64.0, "教辅拆封，不想要了",
            request_id="R-1002", conversation_id="T14")
decide(store, "R-1002", "rejected", "主管",
       "教辅拆封后不退，除非有质量问题；先确认是否缺页或装订脱落", at=NOW)
new_request(store, "A1661", 297.0, "书页破损，按质量问题申请退款",
            request_id="R-1003", conversation_id="CASE-3")
decide(store, "R-1003", "approved", "主管", "迟到批准", at=NOW.replace(hour=14))
# ANCHOR_END: records

# ANCHOR: run
gate = approve_callback(store)
cases = [
    ("R-1001", {"order_id": "A1680", "amount": 680.0, "reason": "装订脱落，按质量问题退全款"}),
    ("R-1002", {"order_id": "A1649", "amount": 64.0, "reason": "教辅拆封，不想要了"}),
    ("R-1003", {"order_id": "A1661", "amount": 297.0, "reason": "书页破损，按质量问题申请退款"}),
]
for request_id, args in cases:
    row = get(store, request_id)
    call = {**args, "approval_id": request_id}
    verdict = gate(None, call)
    print(f"\n{request_id} {args['order_id']} {args['amount']:.2f} 状态={row['status']}")
    print(f"  approve → {verdict!r}")
    if verdict is True:
        out = box.call("issue_refund_approved", call)
        print(f"  工具 → {out}")
    else:
        print(f"  未执行 → 错误：这个操作没有被批准（{verdict}）")

# 绕过 approve 直接调用：工具自己还要再查一次批准单。
bypass = box.call("issue_refund_approved", {**cases[1][1], "approval_id": "R-1002"})
print(f"\n绕过 gate 直接调用 R-1002 → {bypass}")

actions = store.actions()
print(f"\nactions：{len(actions)} 条")
for action in actions:
    detail = action["detail"]
    print(f"  {action['kind']} {action['order_id']} {detail['amount']:.2f} "
          f"approval_id={detail.get('approval_id')} approved_by={detail.get('approved_by')}")
# ANCHOR_END: run
