# 24.3 reducer：两个并行分支在同一个超步写 log
# mode: run
# llm: none
# expect: 合并后 2 条：\['质量问题：60 天内退全款', '退货期限：签收后 15 天内'\]
# expect: 入口给 None.*\['补一条'\]
import operator
import re
from typing import Annotated, TypedDict

from langgraph.graph import END, START, StateGraph

from shop import Store

SECTIONS = {s["title"]: s["text"] for s in Store.policy_sections()}


class State(TypedDict):
    log: Annotated[list[str], operator.add]


def return_window(state: State) -> dict:
    days = re.search(r"签收后 \*\*(\d+) 天\*\*", SECTIONS["退货和退款 / 退货期限"]).group(1)
    return {"log": [f"退货期限：签收后 {days} 天内"]}


def quality_window(state: State) -> dict:
    days = re.search(r"签收后 (\d+) 天内", SECTIONS["退货和退款 / 质量问题"]).group(1)
    return {"log": [f"质量问题：{days} 天内退全款"]}


builder = StateGraph(State)
builder.add_node("退货期限", return_window)
builder.add_node("质量问题", quality_window)
# 两条边都从 START 出发：两个节点在同一个超步里运行
builder.add_edge(START, "退货期限")
builder.add_edge(START, "质量问题")
builder.add_edge("退货期限", END)
builder.add_edge("质量问题", END)
graph = builder.compile()

result = graph.invoke({"log": []})
print(f"合并后 {len(result['log'])} 条：{sorted(result['log'])}")


# 自定义 reducer 要先处理 None，否则入口传 None 会 TypeError
def merge(left: list[str] | None, right: list[str] | None) -> list[str]:
    return (left or []) + (right or [])


class One(TypedDict):
    log: Annotated[list[str], merge]


one = StateGraph(One)
one.add_node("补一条", lambda state: {"log": ["补一条"]})
one.add_edge(START, "补一条")
one.add_edge("补一条", END)
print("自定义 reducer，入口给 None：", one.compile().invoke({"log": None})["log"])
