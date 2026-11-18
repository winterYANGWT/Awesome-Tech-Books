# 20.6 审批审计：按 approval_id 查重复、超时和审批人越级。
# mode: run
# llm: none
# expect: 重复=0
import json

from ch20._lib.approvals import (LEVEL_LIMIT, NOW, _tag_last_action, decide,
                            execute_approved_refund, iso, new_request)
from shop import Store

store = Store()
# 三张单：正常、审批人越级、执行时已过期。
new_request(store, "A1661", 297.0, "书页破损，按质量问题退",
            request_id="R-3001", conversation_id="CASE-3")
decide(store, "R-3001", "approved", "主管", "同意")
execute_approved_refund(store, "A1661", 297.0, "书页破损，按质量问题退", "R-3001")

new_request(store, "A1680", 680.0, "装订脱落，质量问题全款退",
            request_id="R-3002", conversation_id="T13")
decide(store, "R-3002", "approved", "客服", "我批了")
execute_approved_refund(store, "A1680", 680.0, "装订脱落，质量问题全款退", "R-3002")

new_request(store, "A1649", 64.0, "教辅拆封",
            request_id="R-3003", conversation_id="T14")
decide(store, "R-3003", "approved", "主管", "迟到批准", at=NOW.replace(hour=14))
# 模拟一个坏执行：批准单已过期，但退款还是写进了 actions。
store.refund("A1649", 64.0, "教辅拆封", approved_by="主管")
_tag_last_action(store, "R-3003", "主管")
store.db.execute("UPDATE approvals SET executed_at = ? WHERE request_id = 'R-3003'",
                 (iso(NOW.replace(hour=14)),))
store.db.commit()

approvals = store.query("SELECT * FROM approvals ORDER BY request_id")
refunds = [a for a in store.actions() if a["kind"] == "refund"]
by_approval = {}
for action in refunds:
    by_approval.setdefault(action["detail"].get("approval_id"), []).append(action)
duplicate = sum(len(items) - 1 for items in by_approval.values())
timeout = [row["request_id"] for row in approvals
           if row["executed_at"] and row["executed_at"] > row["expires_at"]]
over_level = [row["request_id"] for row in approvals
              if row["status"] == "approved"
              and LEVEL_LIMIT.get(row["approver"], 0) < json.loads(row["args"])["amount"]]

print(f"重复={duplicate}（{len(refunds)} 笔退款，{len(by_approval)} 个 approval_id）")
print(f"超时={len(timeout)}（{', '.join(timeout) or '无'}）")
print(f"越级={len(over_level)}（{', '.join(over_level) or '无'}）")
print("\n审计表：")
print("request_id  order  amount  approver  decision  executed_at          问题")
for row in approvals:
    args = json.loads(row["args"])
    problems = []
    if row["request_id"] in timeout:
        problems.append("超时执行")
    if row["request_id"] in over_level:
        problems.append("越级批准")
    if row["executed_at"] is None:
        problems.append("未执行")
    print(f"{row['request_id']:11} {args['order_id']:6} {args['amount']:7.2f} "
          f"{str(row['approver']):8} {str(row['decision'] or '—'):8} {str(row['executed_at']):20} "
          f"{'、'.join(problems) or '正常'}")
