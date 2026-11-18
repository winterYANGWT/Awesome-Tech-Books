"""只挂资源和提示的 server：5 份政策文档 + 2 个客服话术。"""

from mcp.server.mcpserver import MCPServer
from mcp.server.mcpserver.exceptions import ResourceNotFoundError

from shop import ShopError, Store
from shop.tools import SUPPORT_PROMPT

mcp = MCPServer("shop-policies", version="0.1", instructions=SUPPORT_PROMPT)


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
