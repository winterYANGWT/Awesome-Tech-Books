# 24.3 反例：没有 reducer 时，同一个超步写同一个键会报错
# mode: fail
# llm: none
# expect: Can receive only one value per step
import sys
from typing import TypedDict

from langgraph.errors import InvalidUpdateError
from langgraph.graph import END, START, StateGraph


class State(TypedDict):
    log: list[str]


def return_window(state: State) -> dict:
    return {"log": ["退货期限"]}


def quality_window(state: State) -> dict:
    return {"log": ["质量问题"]}


builder = StateGraph(State)
builder.add_node("退货期限", return_window)
builder.add_node("质量问题", quality_window)
builder.add_edge(START, "退货期限")
builder.add_edge(START, "质量问题")
builder.add_edge("退货期限", END)
builder.add_edge("质量问题", END)
graph = builder.compile()

print("两个分支都写 log，但 State 里的 log 没有 reducer：")
try:
    graph.invoke({"log": []})
except InvalidUpdateError as e:
    print(f"InvalidUpdateError: {str(e).splitlines()[0]}", file=sys.stderr)
    sys.exit(1)
