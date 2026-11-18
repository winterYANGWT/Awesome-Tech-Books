# 27.5 会话：SQLiteSession 保存消息历史，第二轮接着第一轮的退款说。
# mode: run
# expect: 82
import asyncio
import os
from typing import Annotated

from agents import (Agent, Runner, SQLiteSession, function_tool, set_default_openai_api,
                    set_tracing_disabled)
from shop import Store

set_default_openai_api("chat_completions")
set_tracing_disabled(True)
MODEL = os.environ.get("MODEL", "deepseek-flash")
store = Store()


@function_tool
def get_order(order_id: Annotated[str, "订单号，例如 A1659"]) -> dict:
    """查一张订单：状态、实付金额、收货地址和物流。"""
    o = store.order(order_id)
    if o is None:
        raise ValueError(f"订单 {order_id} 不存在")
    s = o["shipment"] or {}
    return {"order_id": o["id"], "status": o["status"], "total_paid": o["total"],
            "courier": s.get("courier"), "latest": s["events"][-1] if s.get("events") else None}


@function_tool
def search_policies(query: Annotated[str, "要查的问题"]) -> list[dict]:
    """在店铺政策里查最相关的条款。"""
    return [{"section": h["title"], "text": h["text"]} for h in store.search_policies(query)]


@function_tool
def issue_refund(order_id: Annotated[str, "订单号"],
                 amount: Annotated[float, "退款金额（元）"],
                 reason: Annotated[str, "退款原因"]) -> dict:
    """给订单退款，钱原路退回。先核对订单和金额。"""
    return store.refund(order_id, amount, reason)


agent = Agent(
    name="support",
    instructions="你是青石书店的客服。先用工具查订单，按退货政策处理缺页等质量问题；"
                 "回答用中文，简洁。今天是 2026-11-17。",
    tools=[get_order, search_policies, issue_refund],
    model=MODEL,
)


async def main():
    session = SQLiteSession("c082", ":memory:")
    for turn, text in enumerate(("收到的《大运河两千年》缺了二十多页，订单号 A1659，怎么处理？",
                                 "刚才那单退款多少？"), 1):
        result = await Runner.run(agent, text, session=session)
        n = len(await session.get_items())
        print(f"第 {turn} 轮：{result.final_output}")
        print(f"（会话里 {n} 条消息）")


asyncio.run(main())
