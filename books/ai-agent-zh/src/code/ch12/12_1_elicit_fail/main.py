# mode: fail
# llm: none
# expect: NoBackChannelError
"""旧写法：工具里直接 await ctx.elicit(...)。2026-07-28 的 stdio 没有回程通道。"""
import asyncio
import os
import sys
from pathlib import Path

from mcp import Client, types
from mcp.client.stdio import StdioServerParameters

HERE = Path(__file__).resolve().parent
SERVER = HERE.parent / "12_1_mrtr" / "server.py"
PARAMS = StdioServerParameters(
    command=sys.executable, args=[str(SERVER)],
    env={"PYTHONPATH": os.environ.get("PYTHONPATH", "")})


async def elicit(ctx, params):
    return types.ElicitResult(action="accept", content={"approved": True, "note": "M01"})


def leaves(exc):
    if isinstance(exc, BaseExceptionGroup):
        for child in exc.exceptions:
            yield from leaves(child)
    else:
        yield exc


async def main():
    failed = False
    try:
        async with Client(PARAMS, elicitation_callback=elicit) as client:
            await client.call_tool("refund_old", {"order_id": "A1680", "amount": 680})
    except BaseExceptionGroup as group:
        failed = True
        print("捕获到 ExceptionGroup，叶子异常：")
        for e in leaves(group):
            print(" ", type(e).__name__, str(e)[:90])
    except Exception as e:  # 有的传输层会直接抛 MCPError
        failed = True
        print("捕获到", type(e).__name__, str(e)[:90])
    if failed:
        print("旧写法失败：server 端 NoBackChannelError（客户端收到 MCPError，没有 back-channel）", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
