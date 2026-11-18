# 24.2 同一张 T06 走四个节点，看 state 的四个键依次出现
# mode: run
# llm: none
# expect: 读工单后.*order=—.*policy=—.*reply=—
# expect: 查订单后.*order=A1659 / 82 元
# expect: 起草后.*reply=.*82
import re
from typing import TypedDict

from langgraph.graph import END, START, StateGraph

from shop import Store, tickets

store = Store()
TICKET = next(t for t in tickets() if t["id"] == "T06")["text"]


class State(TypedDict, total=False):
    ticket: str
    order: dict
    policy: str
    reply: str


def read_ticket(state: State) -> dict:
    return {"ticket": state["ticket"].strip()}


def lookup_order(state: State) -> dict:
    order_id = re.search(r"A\d{4}", state["ticket"]).group()
    o = store.order(order_id)
    return {"order": {"order_id": o["id"], "title": o["items"][0]["title"],
                      "total": o["total"], "status": o["status"]}}


def lookup_policy(state: State) -> dict:
    section = next(s for s in store.search_policies("缺页 质量问题 退货")
                   if "质量问题" in s["title"])
    return {"policy": section["title"]}


def draft_reply(state: State) -> dict:
    o = state["order"]
    return {"reply": f"按《{state['policy']}》，订单 {o['order_id']} 的《{o['title']}》缺页，"
                     f"可以退全款 {o['total']:g} 元。"}


builder = StateGraph(State)
builder.add_node("读工单", read_ticket)
builder.add_node("查订单", lookup_order)
builder.add_node("查政策", lookup_policy)
builder.add_node("起草", draft_reply)
builder.add_edge(START, "读工单")
builder.add_edge("读工单", "查订单")
builder.add_edge("查订单", "查政策")
builder.add_edge("查政策", "起草")
builder.add_edge("起草", END)
graph = builder.compile()


def show(value) -> str:
    if value is None:
        return "—"
    if isinstance(value, dict):
        return f"{value['order_id']} / {value['total']:g} 元 / {value['status']}"
    return value


labels = ["初始", "读工单后", "查订单后", "查政策后", "起草后"]
for label, state in zip(labels, graph.stream({"ticket": TICKET}, stream_mode="values")):
    print(f"{label:6}" + "  ".join(f"{k}={show(state.get(k))}"
                                  for k in ("ticket", "order", "policy", "reply")))
