"""一个把调试信息打到 stdout 的 server。"""

from mcp.server.mcpserver import MCPServer

mcp = MCPServer("bad-stdout", version="0.1")


@mcp.tool()
def ping(text: str = "hi") -> str:
    """回显。注意下面这行 print。"""
    # ANCHOR: print
    print("debug")                 # 这一行进了 stdout，和协议消息混在一起
    # ANCHOR_END: print
    return "pong: " + text


if __name__ == "__main__":
    mcp.run(transport="stdio")
