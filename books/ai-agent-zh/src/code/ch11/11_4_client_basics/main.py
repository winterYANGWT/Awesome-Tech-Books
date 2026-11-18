# llm: none
# expect: is_error
# expect: input_schema

"""看 CallToolResult 的四个部分，以及 ListToolsResult 的 snake_case 属性。"""

import asyncio
import json
import sys
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

CODE = Path(__file__).resolve().parents[2]
SERVER = CODE / "ch11" / "11_2_shop_server" / "server.py"


async def main() -> None:
    params = StdioServerParameters(command=sys.executable, args=[str(SERVER)],
                                   env={"PYTHONPATH": str(CODE)}, cwd=str(CODE))
    async with Client(params) as client:
        tools = (await client.list_tools()).tools
        get_order = next(t for t in tools if t.name == "get_order")
        fields = type(get_order).model_fields
        print("snake_case attrs:", "input_schema" in fields, "output_schema" in fields)
        print("input_schema required:", get_order.input_schema["required"])
        print("wire key inputSchema:", "inputSchema" in get_order.model_dump(by_alias=True))

        ok = await client.call_tool("get_order", {"order_id": "A1670"})
        print("ok content length:", len(ok.content))
        print("ok content[0].type:", ok.content[0].type)
        print("ok text head:", " / ".join(ok.content[0].text.splitlines()[:3]))
        print("ok structured_content:", ok.structured_content)
        print("ok is_error:", ok.is_error)

        bad = await client.call_tool("get_order", {"order_id": "A9999"})
        print("bad is_error:", bad.is_error)
        print("bad text:", bad.content[0].text)


if __name__ == "__main__":
    asyncio.run(main())
