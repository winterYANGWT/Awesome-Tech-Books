# 25.4 从 next=('draft',) 重放，再从同一检查点 fork 成 500，比较两条分支。
# mode: run
# llm: none
# expect: 退款 500.0
"""确定性图：calc(680) → draft → finish；重放和分叉都会重新执行下游节点。"""
from typing import TypedDict

from langgraph.checkpoint.memory import InMemorySaver
from langgraph.graph import END, START, StateGraph

CALLS = {"calc": 0, "draft": 0, "finish": 0}


class State(TypedDict):
    amount: float
    done: str


def calc(state: State):
    CALLS["calc"] += 1
    return {"amount": 680.0}


def draft(state: State):
    CALLS["draft"] += 1
    return {"done": f"退款 {state['amount']:.1f} 待执行"}


def finish(state: State):
    CALLS["finish"] += 1
    return {"done": f"退款 {state['amount']:.1f} 已执行"}


builder = StateGraph(State)
builder.add_node("calc", calc)
builder.add_node("draft", draft)
builder.add_node("finish", finish)
builder.add_edge(START, "calc")
builder.add_edge("calc", "draft")
builder.add_edge("draft", "finish")
builder.add_edge("finish", END)
graph = builder.compile(checkpointer=InMemorySaver())
config = {"configurable": {"thread_id": "T13-travel"}}

graph.invoke({"amount": 0.0, "done": ""}, config)
print("历史（新→旧）：")
for snap in graph.get_state_history(config):
    print(f"  next={snap.next} step={snap.metadata.get('step')} "
          f"amount={snap.values.get('amount', 0):.1f} done={snap.values.get('done', '')!r}")

target = next(s for s in graph.get_state_history(config) if s.next == ("draft",))
print(f"选中 next={target.next} amount={target.values['amount']:.1f}")

replayed = graph.invoke(None, target.config)
print(f"重放：{replayed['done']}")

fork_config = graph.update_state(target.config, {"amount": 500.0}, as_node="calc")
forked = graph.invoke(None, fork_config)
print(f"分叉成 500：{forked['done']}")

finals = sorted({s.values.get("done", "") for s in graph.get_state_history(config) if s.next == ()})
print(f"两条分支的终态：{finals}")
print(f"节点执行次数：{CALLS}")
