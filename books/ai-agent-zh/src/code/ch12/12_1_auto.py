# mode: run
# llm: none
# expect: 批准
# expect: 已退款
"""高层 Client 自动处理 MRTR：业务代码只调一次 call_tool。"""
import asyncio
import json
import os
import sys
from pathlib import Path

from mcp import Client, types
from mcp.client.stdio import StdioServerParameters

HERE = Path(__file__).resolve().parent
PARAMS = StdioServerParameters(
    command=sys.executable, args=[str(HERE / "12_1_mrtr" / "server.py")],
    env={"PYTHONPATH": os.environ.get("PYTHONPATH", "")})


# ANCHOR: callback
async def elicit(ctx, params):
    print("回调 message:", params.message)
    print("requested_schema:", json.dumps(params.requested_schema, ensure_ascii=False, indent=1))
    print("requested_schema 的字段:", list(params.requested_schema.get("properties", {})))
    return types.ElicitResult(action="accept",
                              content={"approved": True, "note": "M01"})
# ANCHOR_END: callback


async def main():
    async with Client(PARAMS, elicitation_callback=elicit) as client:
        print("input_required_max_rounds 默认:", client.input_required_max_rounds)
        result = await client.call_tool("refund", {"order_id": "A1680", "amount": 680})
        print("resultType:", result.result_type, "is_error:", result.is_error)
        print(result.content[0].text)


if __name__ == "__main__":
    asyncio.run(main())
