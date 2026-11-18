# 25.2 没有 checkpointer：invoke 每次都从头跑、每次都返回 __interrupt__，Command 无法恢复。
# mode: fail
# llm: none
# expect: __interrupt__
# expect: Cannot use Command\(resume=...\) without checkpointer
"""没有检查点时，`Command(resume=...)` 抛 RuntimeError；前两次 invoke 都只是重新中断。"""
import sys
from typing import TypedDict

from langgraph.graph import END, START, StateGraph
from langgraph.types import Command, interrupt


class State(TypedDict):
    order_id: str
    approved: bool


def approve(state: State):
    decision = interrupt({"order_id": state["order_id"], "amount": 680.0})
    return {"approved": bool(decision)}


builder = StateGraph(State)
builder.add_node("approve", approve)
builder.add_edge(START, "approve")
builder.add_edge("approve", END)
graph = builder.compile()          # 故意不传 checkpointer
config = {"configurable": {"thread_id": "T13-no-saver"}}

for i in (1, 2):
    out = graph.invoke({"order_id": "A1680", "approved": False}, config)
    print(f"第 {i} 次 invoke：__interrupt__ = {out['__interrupt__'][0].value}")

try:
    graph.invoke(Command(resume=True), config)
except RuntimeError as exc:
    print(f"第 3 次 invoke Command：{exc}", file=sys.stderr)
    raise SystemExit(1)
