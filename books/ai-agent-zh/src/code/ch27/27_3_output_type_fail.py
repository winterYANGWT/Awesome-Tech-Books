# 27.3 反例：output_type 让 SDK 发 json_schema，DeepSeek 返回 400。
# mode: fail
# expect: HTTP 400
# expect: json_schema
import json
import os
import sys
from typing import Literal

from agents import Agent, Runner, set_default_openai_api, set_tracing_disabled
from openai import BadRequestError
from pydantic import BaseModel

set_default_openai_api("chat_completions")
set_tracing_disabled(True)
MODEL = os.environ.get("MODEL", "deepseek-flash")


class TicketCategory(BaseModel):
    category: Literal["物流查询", "退货退款", "修改地址", "政策咨询", "投诉建议", "其他", "越界"]


agent = Agent(
    name="classifier",
    instructions="给青石书店的客服工单分类，只输出类别。",
    output_type=TicketCategory,
    model=MODEL,
)
try:
    result = Runner.run_sync(agent, "收到的《大运河两千年》中间缺了二十多页，订单号 A1659，怎么处理？")
    print(result.final_output)
except BadRequestError as e:
    print(f"HTTP {e.status_code}")
    body = json.loads(e.response.request.content)
    print("SDK 发出的 response_format：", json.dumps(body["response_format"], ensure_ascii=False))
    sys.exit(1)
