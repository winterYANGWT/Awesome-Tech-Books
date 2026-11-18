# 18.2 同一问题两版委派：一句“查一下”缺三样东西，四字段计划才可执行。
# mode: run
# expect: boundaries
import json
import os
from typing import Literal

from openai import OpenAI
from pydantic import BaseModel, Field, ValidationError, model_validator

from ch18.lib.common import parse_json

client = OpenAI()                                  # 读 OPENAI_BASE_URL、OPENAI_API_KEY
MODEL = os.environ.get("MODEL", "deepseek-flash")
NO_THINK = {"thinking": {"type": "disabled"}}
TOOLS = ["readonly_sql", "list_reviews", "search_policies", "read_section"]
QUESTION = "2026 年 Q3 退货率为什么上升，Q4 为什么回落？"


# ANCHOR: plan_model
class Task(BaseModel):
    objective: str = Field(min_length=6)
    output: str = Field(min_length=4)
    tools: list[Literal["readonly_sql", "list_reviews", "search_policies", "read_section"]] = Field(min_length=1)
    boundaries: str = Field(min_length=6)


class Plan(BaseModel):
    tasks: list[Task] = Field(min_length=3, max_length=3)

    @model_validator(mode="after")
    def separate(self):
        if len({t.objective.strip() for t in self.tasks}) != 3:
            raise ValueError("三条线的 objective 不能重复")
        used = [name for t in self.tasks for name in t.tools]
        if len(used) != len(set(used)):
            raise ValueError(f"同一个工具出现在两条线里：{used}")
        for t in self.tasks:
            if not any(w in t.boundaries for w in ("只读", "不改", "不许")):
                raise ValueError(f"boundaries 要写清只读/不改数据：{t.objective}")
        return self
# ANCHOR_END: plan_model


PLAN_PROMPT = f"""你是调研主 agent。把问题拆成恰好 3 个互不重叠的调查任务。
可用工具：readonly_sql（只读 SQL）、list_reviews（读一本书的评论）、search_policies / read_section（查政策）。
每个任务写四个字段：
- objective：查什么，一句话；
- output：返回哪些字段，例如“季度、退货率、样本数”；
- tools：只能用上面 1–2 个工具，三条线的工具不能重复；
- boundaries：时间范围、只读、找不到时写 gap。
只输出 JSON：{{"tasks": [{{"objective": "...", "output": "...", "tools": ["..."], "boundaries": "..."}}]}}"""

VAGUE_PROMPT = "把这个问题交给一个子 agent。只写一句委派说明，例如“查一下退货率为什么上升”，不要列字段。"


def vague() -> str:
    resp = client.chat.completions.create(
        model=MODEL, messages=[{"role": "system", "content": VAGUE_PROMPT}, {"role": "user", "content": QUESTION}],
        max_tokens=120, extra_body=NO_THINK)
    return (resp.choices[0].message.content or "").strip()


def make_plan() -> Plan:
    messages = [{"role": "system", "content": PLAN_PROMPT}, {"role": "user", "content": QUESTION}]
    for attempt in (1, 2):
        resp = client.chat.completions.create(
            model=MODEL, messages=messages, response_format={"type": "json_object"}, max_tokens=1600)
        content = resp.choices[0].message.content or ""
        try:
            return Plan.model_validate(parse_json(content))
        except (ValueError, ValidationError) as e:
            print(f"第 {attempt} 次校验失败：{str(e).splitlines()[0][:80]}")
            messages += [{"role": "assistant", "content": content},
                         {"role": "user", "content": f"校验没过：{e}。按四个字段重写，只输出 JSON。"}]
    raise SystemExit("两次校验都没过")


def main() -> None:
    note = vague()
    print(f"模糊委派：{note}")
    print("字段：objective 只有一句问题；output ✗  tools ✗  boundaries ✗\n")

    plan = make_plan()
    print(f"详细计划：{len(plan.tasks)} 条线，校验通过")
    for i, t in enumerate(plan.tasks, 1):
        print(f"\n[{i}] tools={','.join(t.tools)}")
        print(f"  objective：{t.objective}")
        print(f"  output：{t.output}")
        print(f"  boundaries：{t.boundaries}")
    print(f"\n四条字段齐全：{all(all(getattr(t, f) for f in ('objective', 'output', 'tools', 'boundaries')) for t in plan.tasks)}")
    print(f"三条线工具互不重叠：{len({x for t in plan.tasks for x in t.tools}) == len([x for t in plan.tasks for x in t.tools])}")
    print(f"计划 JSON：{json.dumps(plan.model_dump(), ensure_ascii=False)[:120]}…")


if __name__ == "__main__":
    main()
