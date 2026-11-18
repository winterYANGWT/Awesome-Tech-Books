# llm: none
# expect: issue_refund
# expect: destructiveHint
# expect: is_error

"""列出 9 个工具、schema 和 annotations，再调一次成功和一次失败。"""

import asyncio
import json
import sys
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

CODE = Path(__file__).resolve().parents[2]
SERVER = Path(__file__).with_name("server.py")


async def main() -> None:
    params = StdioServerParameters(command=sys.executable, args=[str(SERVER)],
                                   env={"PYTHONPATH": str(CODE)}, cwd=str(CODE))
    async with Client(params) as client:
        tools = (await client.list_tools()).tools
        print("tools:", len(tools))
        for t in tools:
            required = ", ".join(t.input_schema.get("required", [])) or "-"
            ann = t.annotations.model_dump(by_alias=True, exclude_none=True) if t.annotations else {}
            print(f"  {t.name}: required={required} annotations={json.dumps(ann, ensure_ascii=False) if ann else '-'}")

        ok = await client.call_tool("get_order", {"order_id": "A1670"})
        data = json.loads(ok.content[0].text)
        print("get_order:", data["order_id"], data["shipment"]["courier"], data["status"],
              "is_error=" + str(ok.is_error))

        bad = await client.call_tool("get_order", {"order_id": "A9999"})
        print("missing: is_error=" + str(bad.is_error))
        print("missing text:", bad.content[0].text)


if __name__ == "__main__":
    asyncio.run(main())
