# 20.3 同一个 request_id 恢复两次：第二次必须查 actions，跳过退款。
# mode: run
# llm: none
# expect: 跳过
from ch20._lib.approvals import decide, execute_approved_refund, new_request
from shop import Store

store = Store()
new_request(store, "A1680", 680.0, "装订脱落，按质量问题退全款",
            request_id="R-2001", conversation_id="T13")
decide(store, "R-2001", "approved", "主管", "同意全款退")
args = {"order_id": "A1680", "amount": 680.0, "reason": "装订脱落，按质量问题退全款"}
for attempt in (1, 2):
    result = execute_approved_refund(store, **args, approval_id="R-2001")
    if result["status"] == "已执行过":
        print(f"第 {attempt} 次：{result['status']}，跳过（不再退款）")
    else:
        print(f"第 {attempt} 次：已退款 {result['amount']:.2f}")
refunds = [a for a in store.actions() if a["kind"] == "refund"]
print(f"actions：退款 {len(refunds)} 条")
