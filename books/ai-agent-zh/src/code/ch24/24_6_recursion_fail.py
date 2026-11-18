# 24.6 反例：没有出口的环，recursion_limit 用完就报错
# mode: fail
# llm: none
# expect: Recursion limit of 5
import sys
from typing import TypedDict

from langgraph.errors import GraphRecursionError
from langgraph.graph import START, StateGraph


class State(TypedDict):
    n: int


def tick(state: State) -> dict:
    return {"n": state["n"] + 1}


builder = StateGraph(State)
builder.add_node("tick", tick)
builder.add_edge(START, "tick")
builder.add_edge("tick", "tick")          # 永远回到自己，没有 END
graph = builder.compile()

print("tick 每次都回到 tick，没有停止条件：")
try:
    graph.invoke({"n": 0}, {"recursion_limit": 5})
except GraphRecursionError as e:
    print(f"GraphRecursionError: {str(e).splitlines()[0]}", file=sys.stderr)
    sys.exit(1)
