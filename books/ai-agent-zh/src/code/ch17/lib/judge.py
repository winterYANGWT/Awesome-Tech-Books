"""按 `shop.tickets()` 里的 `expect` 判断一次运行的终态。"""
from __future__ import annotations


def norm(text: str) -> str:
    return str(text).replace(" ", "").replace("\u3000", "")


def problems(ticket: dict, store, answer: str) -> list[str]:
    """返回终态的问题列表；空列表表示正确。"""
    exp = ticket["expect"]
    acts = store.actions()
    out: list[str] = []
    refunds = [a for a in acts if a["kind"] == "refund"]

    if exp["refund"]:
        target = exp["refund"]
        hit = [a for a in refunds if a["order_id"] == target["order_id"]
               and abs(a["detail"]["amount"] - target["amount"]) < 0.01]
        if not hit:
            if refunds:
                out.append(f"退款 {refunds[-1]['detail']['amount']:.2f}，应为 {target['amount']:.2f}")
            else:
                out.append(f"没有退款（应退 {target['amount']:.2f}）")
    elif refunds:
        out.append(f"多退了 {refunds[0]['detail']['amount']:.2f} 元")

    if exp.get("address_changed"):
        target = exp["address_changed"]
        changes = [a for a in acts if a["kind"] == "update_address" and a["order_id"] == target["order_id"]]
        if not changes or norm(target["contains"]) not in norm(changes[-1]["detail"]["new"]):
            out.append("地址没改对")

    if exp.get("escalate") and not any(a["kind"] == "escalate" for a in acts):
        out.append("没有提交主管")

    plain = norm(answer)
    for word in exp["mention"]:
        if norm(word) not in plain:
            out.append(f"没提“{word}”")
    return out


def tool_line(steps: list) -> str:
    """把工具调用压成一行，例如 `get_order(A1536) → issue_refund(A1536, 91.0)`。"""
    parts = []
    for step in steps:
        args = step.arguments
        keep = []
        for key in ("order_id", "amount", "address", "query", "customer_id"):
            if key in args:
                value = str(args[key]).replace("\n", " ")
                if len(value) > 14:
                    value = value[:13] + "…"
                keep.append(value)
        parts.append(f"{step.name}({', '.join(keep)})")
    return " → ".join(parts) or "（没有调工具）"
