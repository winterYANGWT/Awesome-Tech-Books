# 24.1 三个节点：把 harness.run 里的三件事拆开
# mode: run
# llm: none
# expect: steps = 读工单 → 查订单 → 起草回复
# expect: __start__→读工单、读工单→查订单、查订单→起草回复、起草回复→__end__
from typing import TypedDict

from langgraph.graph import END, START, StateGraph


class State(TypedDict):
    steps: list[str]


def read_ticket(state: State) -> dict:
    return {"steps": state["steps"] + ["读工单"]}


def lookup_order(state: State) -> dict:
    return {"steps": state["steps"] + ["查订单"]}


def draft_reply(state: State) -> dict:
    return {"steps": state["steps"] + ["起草回复"]}


builder = StateGraph(State)
builder.add_node("读工单", read_ticket)
builder.add_node("查订单", lookup_order)
builder.add_node("起草回复", draft_reply)
builder.add_edge(START, "读工单")
builder.add_edge("读工单", "查订单")
builder.add_edge("查订单", "起草回复")
builder.add_edge("起草回复", END)
graph = builder.compile()

for i, state in enumerate(graph.stream({"steps": []}, stream_mode="values")):
    print(f"state {i}: {state['steps']}")

print()
print("steps = " + " → ".join(state["steps"]))

# get_graph() 是编译后的结构；__start__ 和 __end__ 是 LangGraph 加上的
g = graph.get_graph()
print("节点：" + "、".join(g.nodes))
names = list(g.nodes)
order = sorted(g.edges, key=lambda e: (names.index(e.source), names.index(e.target)))
print("边：" + "、".join(f"{e.source}→{e.target}" for e in order))
