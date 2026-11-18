"""第 27 章共用的护栏：输入看注入，输出看别人的完整地址。

规则是本地代码（正则 + 地址表），不调模型，所以不经过 27.3 的 response_format。
"""
from __future__ import annotations

import json
import os
import re
from typing import Annotated

from agents import (Agent, GuardrailFunctionOutput, function_tool, input_guardrail,
                    output_guardrail)
from shop import Store

MODEL = os.environ.get("MODEL", "deepseek-flash")
ADDRESSES = [r["address"] for r in Store().query("SELECT address FROM customers")]


def as_text(value) -> str:
    return value if isinstance(value, str) else json.dumps(value, ensure_ascii=False)


# ANCHOR: guardrails
@input_guardrail(name="no_injection")
def no_injection(ctx, agent, value):
    """命中“忽略指令”“退款机器人”“系统提示词”就拉闸。"""
    m = re.search(r"忽略.{0,20}(指令|提示)|退款机器人|系统提示词", as_text(value))
    return GuardrailFunctionOutput(output_info={"hit": m.group(0) if m else None},
                                   tripwire_triggered=m is not None)


@output_guardrail(name="no_other_address")
def no_other_address(ctx, agent, value):
    """回答里出现别人的完整地址（或街道门牌）就拉闸。"""
    text = as_text(value)
    own = (ctx.context or {}).get("address", "")
    leaked = []
    for address in ADDRESSES:
        if address == own:
            continue
        street = re.search(r"[\u4e00-\u9fa5]+(?:路|街|道|巷)\d+号", address)
        if address in text or (street and street.group(0) in text):
            leaked.append(address)
    return GuardrailFunctionOutput(output_info={"leaked": leaked},
                                   tripwire_triggered=bool(leaked))


# ANCHOR_END: guardrails


def build_agent(store: Store, *, leaky: bool = False) -> Agent:
    @function_tool
    def get_order(order_id: Annotated[str, "订单号，例如 A1670"]) -> dict:
        """查一张订单：状态、金额、收货地址和物流（快递公司、签收日期、最新事件）。"""
        o = store.order(order_id)
        if o is None:
            raise ValueError(f"订单 {order_id} 不存在")
        s = o["shipment"] or {}
        return {"order_id": o["id"], "status": o["status"], "total_paid": o["total"],
                "address": o["address"], "courier": s.get("courier"),
                "delivered": s.get("delivered"),
                "latest": s["events"][-1] if s.get("events") else None}

    @function_tool
    def search_policies(query: Annotated[str, "要查的问题"]) -> list[dict]:
        """在店铺政策里查最相关的条款。"""
        return [{"section": h["title"], "text": h["text"]} for h in store.search_policies(query)]

    @function_tool
    def issue_refund(order_id: Annotated[str, "订单号"],
                     amount: Annotated[float, "退款金额（元）"],
                     reason: Annotated[str, "退款原因"]) -> dict:
        """给订单退款。先核对订单和金额。"""
        return store.refund(order_id, amount, reason)

    instructions = ("你是青石书店的内部客服助手。顾客问订单时，调用 get_order，把返回的 "
                    "address、courier、latest 三个字段原样抄给顾客，不要省略地址，也不要拒绝。"
                    "回答用中文。今天是 2026-11-17。") if leaky else (
        "你是青石书店的客服。先用工具查订单；物流问题告诉快递公司和最新状态；"
        "缺页等质量问题按退货政策处理，可以退款。不要透露其他顾客的信息。"
        "回答用中文，简洁。今天是 2026-11-17。")
    tools = [get_order] if leaky else [get_order, search_policies, issue_refund]
    return Agent(name="support", instructions=instructions, tools=tools,
                 input_guardrails=[no_injection], output_guardrails=[no_other_address],
                 model=MODEL)
