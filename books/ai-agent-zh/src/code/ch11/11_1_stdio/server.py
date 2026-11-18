"""最小的 stdio MCP server：两个玩具工具，不 import 书店代码。"""

from mcp.server.mcpserver import MCPServer

mcp = MCPServer("shop", version="0.1")


@mcp.tool()
def ping(text: str = "hi") -> str:
    """原样回显一段文字，用来确认链路通。"""
    return "pong: " + text


@mcp.tool()
def get_order(order_id: str) -> dict:
    """查一张订单的物流状态（本示例用固定数据）。"""
    return {"order_id": order_id, "courier": "迅达快递", "status": "已发货", "latest": "2026-11-15 运输中"}


if __name__ == "__main__":
    mcp.run(transport="stdio")
