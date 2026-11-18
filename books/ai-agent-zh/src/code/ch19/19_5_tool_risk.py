# 19.5 工具风险分级：低自动、中检查、高人工、未知拒绝。
# mode: run
# llm: none
# expect: issue_refund.*human
from shop import Store
from shop.tools import support_tools

store = Store()
box = support_tools(store)
RISK = {
    "find_customer": "low", "get_customer_context": "low", "get_order": "low",
    "search_books": "low", "search_policies": "low", "list_reviews": "low",
    "update_address": "medium", "escalate_to_supervisor": "medium",
    "issue_refund": "high",
}


def gate(tool, args: dict, user_text: str = "") -> tuple[str, str]:
    """返回 (auto/check/human/deny, 原因)。不执行工具。"""
    name = tool if isinstance(tool, str) else tool.name
    risk = RISK.get(name)
    if risk is None:
        return "deny", "未知工具，不在允许列表"
    if risk == "low":
        return "auto", "只读，不改数据"
    if name == "update_address":
        o = store.order(args.get("order_id", ""))
        if o is None:
            return "deny", "订单不存在"
        if o["status"] != "待发货":
            return "deny", f"状态是“{o['status']}”，不能改地址"
        return "check", "待发货；先校验新地址格式和长度"
    if name == "escalate_to_supervisor":
        return "check", "检查备注里有没有手机号、其他顾客信息"
    if name == "issue_refund":
        amount = float(args.get("amount", 0))
        reason = str(args.get("reason", ""))
        o = store.order(args.get("order_id", ""))
        if o is None:
            return "deny", "订单不存在"
        if "系统补偿" in reason or "平台通知" in reason:
            return "human", "退款原因不具体，需人工核实"
        if amount > 500:
            return "human", f"金额 {amount:.0f} 元超过 500"
        if o["status"] in ("待发货", "已取消"):
            return "deny", f"状态是“{o['status']}”，没有可退款项"
        if amount > o["total"] - o["refunded"] + 1e-9:
            return "deny", "超过剩余可退金额"
        return "auto", "金额、状态和原因都符合"
    return "check", "未配置规则，默认人工看一眼"


def approve(tool, args: dict) -> bool | str:
    verdict, reason = gate(tool, args)
    if verdict == "auto":
        return True
    if verdict == "human":
        return f"需要主管批准：{reason}"
    return f"拒绝：{reason}"


CASES = [
    ("issue_refund", {"order_id": "A1680", "amount": 680, "reason": "装订脱落"}),
    ("issue_refund", {"order_id": "A1340", "amount": 100, "reason": "系统补偿"}),
    ("get_order", {"order_id": "A1001"}),
    ("update_address", {"order_id": "A1679", "address": "天津市文化路 200 号"}),
    ("delete_order", {"order_id": "A1680"}),
]

print("调用                                    级别    决定   原因")
for name, args in CASES:
    tool = box.tools.get(name, name)
    verdict, reason = gate(tool, args)
    level = RISK.get(name, "—")
    print(f"{name}({args['order_id']})".ljust(38) + f" {level:<6} {verdict:<6} {reason}")
print()
print("approve(issue_refund, 680) →", approve(box.tools["issue_refund"], CASES[0][1]))
print("actions：", store.actions())
