# 7.3 编排者提交 3 个任务，Pydantic 校验数量、角色、输入和依赖。
# mode: run
# expect: submit_plan
import json
import os
from typing import Literal

from openai import OpenAI
from pydantic import BaseModel, Field, model_validator

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
Role = Literal["订单查询", "政策查询", "评论查询", "回复草稿"]


# ANCHOR: model
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


# ANCHOR_END: model
PLAN_TOOL = {
    "type": "function",
    "function": {
        "name": "submit_plan",
        "description": "提交工单处理计划，只提交任务，不执行操作。",
        "parameters": {
            "type": "object",
            "properties": {"tasks": {"type": "array", "minItems": 1, "maxItems": 4, "items": {
                "type": "object",
                "properties": {
                    "id": {"type": "string", "enum": ["T1", "T2", "T3", "T4"]},
                    "role": {"type": "string", "enum": list(Role.__args__)},
                    "goal": {"type": "string", "minLength": 4},
                    "inputs": {"type": "object", "properties": {
                        "order_id": {"type": "string"}, "book_id": {"type": "string"},
                        "depends_on": {"type": "array", "items": {"type": "string"}}}},
                    "output_schema": {"type": "string",
                                      "enum": ["order_facts", "policy_clauses",
                                               "review_digest", "reply_draft"]},
                },
                "required": ["id", "role", "goal", "inputs", "output_schema"]}}},
            "required": ["tasks"],
        },
    },
}


# ANCHOR: run
def main() -> None:
    prompt = """为工单 T06（订单 A1659，《大运河两千年》缺页）制定恰好 3 个任务：
T1 订单查询，T2 政策查询，T3 回复草稿。
T3 的 inputs 里写 "depends_on": ["T1", "T2"]，因为回复要用订单事实和政策条款。
用 submit_plan 提交，不要执行退款。"""
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "user", "content": prompt}],
        tools=[PLAN_TOOL], tool_choice={"type": "function", "function": {"name": "submit_plan"}},
        extra_body={"thinking": {"type": "disabled"}})
    call = resp.choices[0].message.tool_calls[0]
    raw = json.loads(call.function.arguments)
    print("submit_plan 原始参数：")
    print(json.dumps(raw, ensure_ascii=False, indent=1)[:900])

    plan = Plan.model_validate(raw)
    print(f"Pydantic 校验通过：{len(plan.tasks)} 个任务")
    for t in plan.tasks:
        deps = t.inputs.get("depends_on", [])
        print(f"  {t.id} {t.role} schema={t.output_schema} "
              f"inputs={json.dumps(t.inputs, ensure_ascii=False)} 依赖={','.join(deps) or '无'}")
    print(f"T3 依赖 T1/T2：{'是' if 'T1' in plan.tasks[-1].inputs.get('depends_on', []) else '否'}")


if __name__ == "__main__":
    main()
# ANCHOR_END: run
