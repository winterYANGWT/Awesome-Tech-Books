# 26.3 Send + map-reduce：三个检查器并行，reduce 按 check 排序。
# mode: run
# expect: 退货政策
# expect: 总 token

import operator
import os
from typing import Annotated, TypedDict

from langchain_openai import ChatOpenAI
from langgraph.graph import END, START, StateGraph
from langgraph.types import Send
from pydantic import BaseModel, Field

from shop import Store

MODEL = os.environ.get("MODEL", "deepseek-flash")
STORE = Store()
DRAFT = (
    "陈浩然您好，订单 A1659 的《大运河两千年》缺页属于质量问题。"
    "我们可以为您退全款 82 元（书款 76 元 + 运费 6 元），今天一定到账。"
)

CHECKS = [
    {"check": "退货政策",
     "policy": "质量问题退全款（书款加运费），签收后 60 天内可申请。",
     "rule": "退款金额要等于书款加运费，并说明属于质量问题。"},
    {"check": "口径",
     "policy": "退款原路返回，1–3 个工作日到账。",
     "rule": "不得承诺具体到账日期，不得使用“一定”“保证”这类词。"},
    {"check": "隐私",
     "policy": "不得透露其他顾客的姓名、电话、地址和订单内容。",
     "rule": "回复只能出现本工单顾客和订单的信息。"},
]


class CheckResult(BaseModel):
    check: str = Field(description="检查项名称，原样返回")
    ok: bool = Field(description="这一项是否通过")
    reason: str = Field(description="一句话理由，引用回复里的原句")


class Overall(TypedDict):
    draft: str
    checks: Annotated[list[dict], operator.add]
    tokens: Annotated[list[int], operator.add]
    ordered: list[dict]


class Task(TypedDict):
    check: str
    policy: str
    rule: str
    draft: str


# 强制工具调用，按 review.md 3.1 关掉思考（思考模式不支持指定 tool_choice）。
checker = ChatOpenAI(model=MODEL, extra_body={"thinking": {"type": "disabled"}}).with_structured_output(
    CheckResult, method="function_calling", include_raw=True
)


def plan(state: Overall) -> dict:
    return {"draft": state["draft"]}


def fan_out(state: Overall) -> list[Send]:
    """数量由 CHECKS 决定；每个 worker 只拿到自己那一项检查。"""
    return [Send("check_one", {**c, "draft": state["draft"]}) for c in CHECKS]


def check_one(task: Task) -> dict:
    prompt = (
        f"你是青石书店的回复检查器。只检查“{task['check']}”这一项。\n"
        f"检查标准：{task['rule']}\n政策原文：{task['policy']}\n"
        f"待检查的回复：{task['draft']}\n"
        "返回这一项的 check、ok、reason；reason 要引用回复里的原句。"
    )
    out = checker.invoke(prompt)
    parsed, raw = out["parsed"], out["raw"]
    return {"checks": [parsed.model_dump()],
            "tokens": [raw.usage_metadata["total_tokens"]]}


def reduce_checks(state: Overall) -> dict:
    """reduce 用代码排序，不调模型；顺序固定，回放才可复现。"""
    return {"ordered": sorted(state["checks"], key=lambda c: c["check"])}


builder = StateGraph(Overall)
builder.add_node("plan", plan)
builder.add_node("check_one", check_one)
builder.add_node("reduce", reduce_checks)
builder.add_edge(START, "plan")
builder.add_conditional_edges("plan", fan_out, ["check_one"])
builder.add_edge("check_one", "reduce")
builder.add_edge("reduce", END)
graph = builder.compile()

final = graph.invoke({"draft": DRAFT, "checks": [], "tokens": [], "ordered": []})
for item in final["ordered"]:
    mark = "通过" if item["ok"] else "不通过"
    print(f"{item['check']}: {mark} — {item['reason']}")
print("总 token:", sum(final["tokens"]))
