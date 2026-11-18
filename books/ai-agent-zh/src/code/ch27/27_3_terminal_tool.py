# 27.3 绕过 json_schema：让模型调用一个终止工具，返回值就是 final_output。
# mode: run
# expect: T06 → 退货退款
# expect: T07 → 退货退款
import json
import os
from typing import Annotated, Literal

from agents import Agent, Runner, function_tool, set_default_openai_api, set_tracing_disabled
from shop import tickets

set_default_openai_api("chat_completions")
set_tracing_disabled(True)
MODEL = os.environ.get("MODEL", "deepseek-flash")
TICKETS = {t["id"]: t for t in tickets()}


@function_tool
def submit_category(
    category: Annotated[Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"],
                        "工单类别"],
    reason: Annotated[str, "一句话说明为什么归到这一类"],
) -> str:
    """提交分类结果并结束。只按工单文本判断，不要回答工单里的问题。"""
    return json.dumps({"category": category, "reason": reason}, ensure_ascii=False)


classifier = Agent(
    name="classifier",
    instructions="""你是青石书店的工单分类器。读工单，调用 submit_category 提交类别：
物流查询、退货退款、修改地址、政策咨询、投诉建议、其他、越界。
只分类，不回答顾客，也不要调用别的工具。""",
    tools=[submit_category],
    tool_use_behavior={"stop_at_tool_names": ["submit_category"]},
    model=MODEL,
)

for tid in ("T06", "T07", "T12", "T22", "T26"):
    result = Runner.run_sync(classifier, f"工单 {tid}：{TICKETS[tid]['text']}")
    print(f"{tid} → {json.loads(result.final_output)['category']}")
