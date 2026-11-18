# 27.1 最小的 Agents SDK 程序：一个工具、一次物流查询。
# mode: run
# expect: last_agent=bookshop
# expect: get_order
# expect: 迅达快递
import json
import os
from typing import Annotated

from agents import Agent, Runner, function_tool, set_default_openai_api, set_tracing_disabled
from shop import Store

set_default_openai_api("chat_completions")
set_tracing_disabled(True)
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()


@function_tool
def get_order(order_id: Annotated[str, "订单号，例如 A1670"]) -> dict:
    """查一张订单：状态、实付金额、收货地址和物流（快递公司、签收日期、最新事件）。"""
    o = store.order(order_id)
    if o is None:
        raise ValueError(f"订单 {order_id} 不存在")
    s = o["shipment"] or {}
    return {"order_id": o["id"], "status": o["status"], "total_paid": o["total"],
            "courier": s.get("courier"), "delivered": s.get("delivered"),
            "latest": s["events"][-1] if s.get("events") else None}


agent = Agent(
    name="bookshop",
    instructions="你是青石书店的客服。查订单和物流要用 get_order，回答用中文，"
                 "简洁，告诉顾客快递公司和最新状态。今天是 2026-11-17。",
    tools=[get_order],
    model=MODEL,
)
result = Runner.run_sync(agent, "我的订单 A1670 到哪了？什么时候能到？")

print(f"last_agent={result.last_agent.name}")
for item in result.new_items:
    if item.type == "tool_call_item":
        args = json.loads(item.raw_item.arguments or "{}")
        print(f"工具调用：{item.raw_item.name}({args})")
print(f"final_output={result.final_output}")
