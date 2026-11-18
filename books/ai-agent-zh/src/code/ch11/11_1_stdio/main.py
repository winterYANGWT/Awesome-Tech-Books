# llm: none
# expect: 2026-07-28
# expect: get_order

"""用 Client 启动 server 子进程，列工具，再调用 get_order。"""

import asyncio
import json
import sys
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

SERVER = Path(__file__).with_name("server.py")


async def main() -> None:
    # ANCHOR: client_short
    params = StdioServerParameters(command=sys.executable, args=[str(SERVER)])
    async with Client(params) as client:          # 进入 async with 才启动子进程
        print("protocol_version:", client.protocol_version)
        print("server_info:", client.server_info.name, client.server_info.version)

        tools = (await client.list_tools()).tools
        print("tools:", ", ".join(t.name for t in tools))
        get_order = next(t for t in tools if t.name == "get_order")
        print("get_order required:", get_order.input_schema["required"])
        print("get_order properties:", sorted(get_order.input_schema["properties"]))

        result = await client.call_tool("get_order", {"order_id": "A1670"})
        # ANCHOR_END: client_short
        data = json.loads(result.content[0].text)
        print("result:", data["order_id"], data["courier"], data["status"])
        print("is_error:", result.is_error)


if __name__ == "__main__":
    asyncio.run(main())
