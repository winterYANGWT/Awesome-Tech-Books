"""书店 MCP server：9 个客服工具、5 份政策资源、2 个话术提示。

工具函数来自 `shop.tools.support_tools`；这里只做三件事：
1. 把 harness 的 Tool 注册给 MCPServer；
2. 把 `harness.ToolError` / `shop.ShopError` 转成 MCP 的 `ToolError`；
3. 加上符合规范的 annotations。
"""

import inspect
import typing

from mcp.server.mcpserver import MCPServer
from mcp.server.mcpserver.exceptions import ResourceNotFoundError, ToolError as MCPToolError
from mcp.types import ToolAnnotations

from harness import ToolError as HarnessToolError
from shop import ShopError, Store
from shop.tools import SUPPORT_PROMPT, support_tools

mcp = MCPServer("shop", version="0.1", instructions=SUPPORT_PROMPT)
store = Store()
box = support_tools(store)

READ_ONLY = {"find_customer", "get_customer_context", "get_order", "search_books",
             "search_policies", "list_reviews"}
DESTRUCTIVE = {"issue_refund", "update_address"}


# ANCHOR: wrap
def wrap(tool):
    """保留原函数的签名和 docstring，只换掉异常类型。"""
    sig = inspect.signature(tool.fn)
    hints = typing.get_type_hints(tool.fn, include_extras=True)
    params = [p.replace(annotation=hints.get(p.name, p.annotation)) for p in sig.parameters.values()]
    sig = sig.replace(parameters=params, return_annotation=hints.get("return", inspect.Signature.empty))

    # ANCHOR: wrap_core
    def call(*args, **kwargs):
        try:
            return tool.fn(*args, **kwargs)
        except (HarnessToolError, ShopError) as e:
            raise MCPToolError(str(e)) from None
    # ANCHOR_END: wrap_core

    call.__name__ = tool.name
    call.__doc__ = tool.description
    call.__signature__ = sig
    return call


for tool in box.tools.values():
    annotations = None
    if tool.name in READ_ONLY:
        annotations = ToolAnnotations(read_only_hint=True, open_world_hint=False)
    elif tool.name in DESTRUCTIVE:
        annotations = ToolAnnotations(destructive_hint=True, open_world_hint=False)
    elif tool.name == "escalate_to_supervisor":
        annotations = ToolAnnotations(destructive_hint=False, open_world_hint=False)
    mcp.add_tool(wrap(tool), name=tool.name, description=tool.description, annotations=annotations)
# ANCHOR_END: wrap


# ANCHOR: resource
@mcp.resource("shop://policies/{name}", name="policy", description="按名字读一份政策文档",
              mime_type="text/markdown")
def policy(name: str) -> str:
    """返回政策文档的 Markdown 全文；名字来自 URI 模板。"""
    try:
        return Store.policy(name)
    except ShopError as e:
        raise ResourceNotFoundError(str(e)) from None
# ANCHOR_END: resource


# ANCHOR: prompt
@mcp.prompt(name="refund_guidance", description="核对订单和政策后，写退款回复")
def refund_guidance(order_id: str, amount: float) -> str:
    return (f"订单 {order_id} 应退 {amount:.2f} 元。先读 shop://policies/returns，"
            "确认签收日期和商品状态，再告诉顾客退款金额和下一步。")


@mcp.prompt(name="shipping_reply", description="查物流后，写配送回复")
def shipping_reply(order_id: str, courier: str) -> str:
    return f"订单 {order_id} 由{courier}配送。先查物流，再告诉顾客最新节点和预计送达时间。"
# ANCHOR_END: prompt


if __name__ == "__main__":
    mcp.run(transport="stdio")
