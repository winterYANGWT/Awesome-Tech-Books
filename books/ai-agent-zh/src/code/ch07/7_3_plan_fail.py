# 7.3 反例：同样的 Plan 校验规则先拒绝 6 个任务，再拒绝重复 id，退出码 1。
# mode: fail
# llm: none
# expect: 最多 4
# expect: 重复
import sys
from typing import Literal

from pydantic import BaseModel, Field, ValidationError, model_validator

Role = Literal["订单查询", "政策查询", "评论查询", "回复草稿"]


class Task(BaseModel):
    id: Literal["T1", "T2", "T3", "T4"]
    role: Role
    goal: str = Field(min_length=4, max_length=120)
    inputs: dict[str, str | list[str]]
    output_schema: Literal["order_facts", "policy_clauses", "review_digest", "reply_draft"]

    @model_validator(mode="after")
    def check_inputs(self):
        if not ({"order_id", "book_id"} & set(self.inputs)):
            raise ValueError("inputs 至少要有 order_id 或 book_id")
        if not self.goal.strip():
            raise ValueError("goal 不能为空")
        return self


class Plan(BaseModel):
    tasks: list[Task] = Field(min_length=1, max_length=4)

    @model_validator(mode="after")
    def check_plan(self):
        if len(self.tasks) > 4:
            raise ValueError("最多 4 个任务，请合并成 3 个")
        ids = [t.id for t in self.tasks]
        if len(ids) != len(set(ids)):
            raise ValueError(f"任务 id 重复：{ids}")
        for t in self.tasks:
            for dep in t.inputs.get("depends_on", []):
                if dep not in ids:
                    raise ValueError(f"{t.id} 依赖不存在的任务 {dep}")
        return self


def rejected(tasks: list[dict]) -> str:
    try:
        Plan.model_validate({"tasks": tasks})
    except ValidationError as e:
        msg = e.errors()[0]["msg"].removeprefix("Value error, ")
        return "最多 4 个任务，请合并成 3 个" if "at most 4 items" in msg else msg
    return "（没有拒绝）"


# ANCHOR: reject
six = [
    {"id": "T1", "role": "订单查询", "goal": "查订单事实", "inputs": {"order_id": "A1659"},
     "output_schema": "order_facts"},
    {"id": "T2", "role": "政策查询", "goal": "查退货政策", "inputs": {"order_id": "A1659"},
     "output_schema": "policy_clauses"},
    {"id": "T3", "role": "评论查询", "goal": "查 B12 评论", "inputs": {"book_id": "B12"},
     "output_schema": "review_digest"},
    {"id": "T4", "role": "回复草稿", "goal": "写回复", "inputs": {"order_id": "A1659", "depends_on": ["T1", "T2"]},
     "output_schema": "reply_draft"},
    {"id": "T1", "role": "订单查询", "goal": "再查一次订单", "inputs": {"order_id": "A1659"},
     "output_schema": "order_facts"},
    {"id": "T2", "role": "政策查询", "goal": "再查一次政策", "inputs": {"order_id": "A1659"},
     "output_schema": "policy_clauses"},
]
print(f"6 个任务：计划被拒绝：{rejected(six)}")

two = [
    {"id": "T1", "role": "订单查询", "goal": "查订单事实", "inputs": {"order_id": "A1659"},
     "output_schema": "order_facts"},
    {"id": "T1", "role": "政策查询", "goal": "查退货政策", "inputs": {"order_id": "A1659"},
     "output_schema": "policy_clauses"},
]
print(f"2 个任务都叫 T1：计划被拒绝：{rejected(two)}", file=sys.stderr)
raise SystemExit(1)
# ANCHOR_END: reject
