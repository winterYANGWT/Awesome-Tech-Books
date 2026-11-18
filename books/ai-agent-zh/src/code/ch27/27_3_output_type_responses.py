# 27.3 对照：不切 Chat Completions，Responses 模式下 output_type 直接可用。
# mode: run
# expect: category=退货退款
import os
from typing import Literal

from agents import Agent, Runner, set_default_openai_client, set_tracing_disabled
from openai import AsyncOpenAI
from pydantic import BaseModel
from shop import tickets

set_default_openai_client(AsyncOpenAI())     # 读 OPENAI_BASE_URL、OPENAI_API_KEY；0.24 默认走 Responses
set_tracing_disabled(True)
MODEL = os.environ.get("MODEL", "deepseek-flash")
T06 = next(t for t in tickets() if t["id"] == "T06")


class TicketCategory(BaseModel):
    category: Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]


agent = Agent(
    name="classifier",
    instructions="给青石书店的客服工单分类，只输出类别。",
    output_type=TicketCategory,
    model=MODEL,
)
result = Runner.run_sync(agent, f"工单 T06：{T06['text']}")
print(f"final_output 的类型：{type(result.final_output).__name__}")
print(f"T06 → category={result.final_output.category}")
