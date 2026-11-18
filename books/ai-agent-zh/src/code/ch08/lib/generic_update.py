"""两版改地址：A 是通用 update(order_id, field, value)；B 是专用 update_address。"""
from __future__ import annotations

from typing import Annotated

from harness import ToolError, Toolbox, tool
from shop import ShopError, Store
from shop.tools import support_tools


def update_tools(store: Store, dedicated: bool) -> Toolbox:
    get_order = support_tools(store, include=("get_order",)).tools["get_order"]

    def guard(fn):
        def inner(*a, **k):
            try:
                return fn(*a, **k)
            except ShopError as e:
                raise ToolError(str(e)) from None
        return inner

    if dedicated:
        @tool(risky=True)
        def update_address(order_id: Annotated[str, "订单号"],
                           address: Annotated[str, "新的完整收货地址"]) -> dict:
            """修改订单的收货地址。只有“待发货”的订单能改。"""
            return guard(store.update_address)(order_id, address)
    else:
        @tool(risky=True)
        def update(order_id: str, field: str, value: str) -> dict:
            """更新订单的一个字段。"""
            if field not in ("addr", "status"):
                raise ToolError(f"未知字段 {field}；这张表的列名是 addr、status")
            if field == "status":
                raise ToolError("status 不能直接改；发货状态由仓库更新")
            return guard(store.update_address)(order_id, value)

    return Toolbox([get_order, update_address if dedicated else update])
