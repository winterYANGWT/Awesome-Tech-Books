# 26.1 子图作为节点：子图内部有 policy_text，父图只看 verdict/reason。
# mode: run
# llm: none
# expect: verdict
# expect: policy_text

from typing import TypedDict

from langgraph.graph import END, START, StateGraph

from shop import Store

STORE = Store()


class SubState(TypedDict):
    order: dict
    policy_name: str
    issue: str
    policy_text: str
    verdict: str
    reason: str


class SubOutput(TypedDict):
    verdict: str
    reason: str


class ParentState(TypedDict):
    order: dict
    policy_name: str
    issue: str
    policy_text: str
    verdict: str
    reason: str
    draft: str


def search_policy(state: SubState) -> dict:
    """子图私有步骤：把政策原文读进 policy_text。"""
    text = Store.policy(state["policy_name"])
    section = text.split("## 质量问题", 1)[1].split("## 非质量问题", 1)[0]
    return {"policy_text": "质量问题" + section.strip()}


def judge(state: SubState) -> dict:
    """只根据订单事实和政策原文判断，不调用模型。"""
    order = state["order"]
    quality = "缺页" in state["issue"] or order["shipment"]["damaged"]
    if "质量问题" in state["policy_text"] and quality:
        return {"verdict": "符合", "reason": f"质量问题退全款，应退 {order['total']:.2f} 元（含运费）"}
    return {"verdict": "不符合", "reason": "没有找到质量问题的依据"}


sub_builder = StateGraph(SubState, output_schema=SubOutput)
sub_builder.add_node("search", search_policy)
sub_builder.add_node("judge", judge)
sub_builder.add_edge(START, "search")
sub_builder.add_edge("search", "judge")
sub = sub_builder.compile()


def load(state: ParentState) -> dict:
    """父图只把订单和要查的政策名交给子图。"""
    return {"order": STORE.order("A1659"), "policy_name": "returns", "issue": "缺页"}


def draft(state: ParentState) -> dict:
    return {"draft": f"T06 政策检查：{state['verdict']}。{state['reason']}"}


builder = StateGraph(ParentState)
builder.add_node("load", load)
builder.add_node("policy_check", sub)
builder.add_node("draft", draft)
builder.add_edge(START, "load")
builder.add_edge("load", "policy_check")
builder.add_edge("policy_check", "draft")
builder.add_edge("draft", END)
graph = builder.compile()

final = graph.invoke({"order": {}, "policy_name": "", "issue": "", "policy_text": "", "verdict": "", "reason": "", "draft": ""})
print("父图输出键:", ", ".join(sorted(k for k in final if not k.startswith("__"))))
print("verdict:", final["verdict"])
print("reason:", final["reason"])
print("draft:", final["draft"])
print("policy_text:", repr(final["policy_text"]), "← 子图里写过，父图仍是空串")
