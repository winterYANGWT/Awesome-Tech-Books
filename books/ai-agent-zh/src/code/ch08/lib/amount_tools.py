"""两版退款工具：A 收 amount 让模型自己算；B 收 kind，金额在函数里按政策算。"""
from __future__ import annotations

from typing import Annotated, Literal

from harness import ToolError, Toolbox, tool
from shop import REFUND_LIMIT, ShopError, Store
from shop.tools import support_tools


def amount_tools(store: Store, design: str) -> Toolbox:
    def guard(fn):
        def inner(*a, **k):
            try:
                return fn(*a, **k)
            except ShopError as e:
                raise ToolError(str(e)) from None
        return inner

    if design == "自算金额":
        @tool
        def get_order_items(order_id: Annotated[str, "订单号"]) -> dict:
            """查订单的商品明细和每本书的实付金额。"""
            o = store.order(order_id)
            if o is None:
                raise ToolError(f"订单 {order_id} 不存在")
            items = [{"title": i["title"], "paid": round(i["price"] * i["qty"] * o["discount"], 2)}
                     for i in o["items"]]
            return {"order_id": o["id"], "status": o["status"],
                    "items": items, "items_total": round(sum(i["paid"] for i in items), 2)}

        @tool(risky=True)
        def issue_refund(order_id: Annotated[str, "订单号"],
                         amount: Annotated[float, "退款金额（元），由你根据订单计算"],
                         reason: Annotated[str, "退款原因"]) -> dict:
            """给订单退款。金额由调用方计算后传入，工具不核对政策。"""
            return guard(store.refund)(order_id, amount, reason)
        return Toolbox([get_order_items, issue_refund])
    else:
        get_order = support_tools(store, include=("get_order",)).tools["get_order"]

        @tool(risky=True)
        def issue_refund(order_id: Annotated[str, "订单号"],
                         kind: Annotated[Literal["质量问题", "不想要了"], "退款类别"],
                         reason: Annotated[str, "退款原因"]) -> dict:
            """给订单退款。金额由工具按政策计算，不要自己传金额。
            质量问题退全款（书款加运费）；不想要了只退书款，运费不退。
            单笔超过 500 元要提交主管。"""
            o = store.order(order_id)
            if o is None:
                raise ToolError(f"订单 {order_id} 不存在")
            if kind == "质量问题":
                amount = round(o["total"] - o["refunded"], 2)
            else:
                amount = round(sum(round(i["price"] * i["qty"] * o["discount"], 2)
                                   for i in o["items"]), 2)
            assert amount <= REFUND_LIMIT + 1e-9      # 演示用，真实规则由 store 再检查
            return guard(store.refund)(order_id, amount, reason)

        return Toolbox([get_order, issue_refund])
