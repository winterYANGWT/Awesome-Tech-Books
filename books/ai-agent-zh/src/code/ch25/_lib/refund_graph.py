"""25.2/25.3 共用的退款图：读订单 → 中断等批准 → 执行 → 写回复。

金额由订单数据算出来，不让模型猜；只有最后的回复交给模型。

节点名：propose（算金额）→ 超过 500 元才进 approve（interrupt）→ execute（退款或提交主管）→ report（模型写回复）。
"""
from __future__ import annotations

import os
from typing import TypedDict

from langchain_openai import ChatOpenAI
from langgraph.graph import END, START, StateGraph
from langgraph.types import interrupt

from shop import REFUND_LIMIT, Store


class RefundState(TypedDict):
    order_id: str
    amount: float
    reason: str
    approved: bool
    by: str
    note: str
    edited_by: str
    edit_note: str
    outcome: dict
    reply: str


SYSTEM = """你是青石书店的客服。根据给定的处理结果写一段给顾客的回复。
说清楚退了多少钱、多久到账、还需要顾客做什么；不要新增没有发生过的操作，不要提内部表名。"""


def build(store: Store, *, with_report: bool = True, checkpointer=None):
    model = ChatOpenAI(model=os.environ.get("MODEL", "deepseek-flash"))

    # ANCHOR: nodes
    def propose(state: RefundState):
        order = store.order(state["order_id"])
        amount = round(order["total"] - order["refunded"], 2)
        return {"amount": amount}

    def approve(state: RefundState):
        decision = interrupt({"order_id": state["order_id"], "amount": state["amount"],
                              "reason": state["reason"]})
        if isinstance(decision, dict):
            return {"approved": bool(decision.get("approved")),
                    "by": decision.get("by", ""),
                    "note": decision.get("note", "")}
        return {"approved": bool(decision), "by": "主管" if decision else "",
                "note": "主管同意" if decision else "主管拒绝"}

    def execute(state: RefundState):
        if state["approved"] or state["amount"] <= REFUND_LIMIT:
            approver = state["by"] or "自动"
            outcome = store.refund(state["order_id"], state["amount"], state["reason"],
                                   approved_by=approver)
            return {"outcome": outcome, "approved": True, "by": approver}
        outcome = store.escalate(state["order_id"], state["note"])
        return {"outcome": outcome}
    # ANCHOR_END: nodes

    def route_after_propose(state: RefundState):
        return "approve" if state["amount"] > REFUND_LIMIT else "execute"

    def report(state: RefundState):
        if state["approved"]:
            facts = f"已退款 {state['amount']:.2f} 元，批准人：{state['by']}。"
        else:
            facts = f"没有退款；已提交主管，原因：{state['note']}。"
        msg = model.invoke([("system", SYSTEM),
                            ("user", f"订单 {state['order_id']}，顾客原因：{state['reason']}。{facts}")])
        return {"reply": msg.content}

    builder = StateGraph(RefundState)
    builder.add_node("propose", propose)
    builder.add_node("approve", approve)
    builder.add_node("execute", execute)
    builder.add_edge(START, "propose")
    builder.add_conditional_edges("propose", route_after_propose,
                                  {"approve": "approve", "execute": "execute"})
    builder.add_edge("approve", "execute")
    if with_report:
        builder.add_node("report", report)
        builder.add_edge("execute", "report")
        builder.add_edge("report", END)
    else:
        builder.add_edge("execute", END)
    return builder.compile(checkpointer=checkpointer)
