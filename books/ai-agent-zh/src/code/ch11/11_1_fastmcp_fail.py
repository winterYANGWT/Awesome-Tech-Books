# mode: fail
# llm: none
# expect: ModuleNotFoundError
# expect: MCPServer
"""照 2025 年的教程写：从 mcp.server.fastmcp 导入 FastMCP。mcp 2.3 没有这个模块。"""

from mcp.server.fastmcp import FastMCP

mcp = FastMCP("shop")
