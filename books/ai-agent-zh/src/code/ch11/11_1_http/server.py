"""Streamable HTTP server：一个 endpoint，两个玩具工具；middleware 记录请求头。"""

from mcp.server.mcpserver import MCPServer

mcp = MCPServer("shop-http", version="0.1")
seen: list[dict] = []


@mcp.tool()
def ping(text: str = "hi") -> str:
    """原样回显一段文字，用来确认链路通。"""
    return "pong: " + text


@mcp.tool()
def get_order(order_id: str) -> dict:
    """查一张订单的物流状态（本示例用固定数据）。"""
    return {"order_id": order_id, "courier": "迅达快递", "status": "已发货"}


# ANCHOR: middleware
async def record_headers(ctx, call_next):
    if ctx.method in ("tools/list", "tools/call"):
        h = ctx.request.headers
        seen.append({"method": ctx.method, "accept": h.get("accept", ""),
                     "version": h.get("mcp-protocol-version", ""),
                     "mcp_method": h.get("mcp-method", ""), "mcp_name": h.get("mcp-name", "")})
    return await call_next(ctx)
# ANCHOR_END: middleware


mcp.middleware.append(record_headers)


def app():
    """给 uvicorn 的 ASGI app；默认 path 是 /mcp。"""
    return mcp.streamable_http_app(host="127.0.0.1")
