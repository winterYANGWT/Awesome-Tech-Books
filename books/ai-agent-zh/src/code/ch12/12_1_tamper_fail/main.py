# mode: fail
# llm: none
# expect: requestState
# expect: 校验失败
"""把 requestState 改一个字符再重试：server 的完整性校验必须拒绝。"""
import asyncio
import os
import sys
from pathlib import Path

from mcp import Client, types
from mcp.client.stdio import StdioServerParameters
from mcp.shared.exceptions import MCPError

HERE = Path(__file__).resolve().parent
SERVER = HERE.parent / "12_1_mrtr" / "server.py"
PARAMS = StdioServerParameters(
    command=sys.executable, args=[str(SERVER)],
    env={"PYTHONPATH": os.environ.get("PYTHONPATH", "")})
ASK = {"order_id": "A1680", "amount": 680}


async def elicit(ctx, params):
    return types.ElicitResult(action="accept", content={"approved": True, "note": "M01"})


async def main():
    failed = False
    async with Client(PARAMS, elicitation_callback=elicit) as client:
        first = await client.session.call_tool("refund", ASK, allow_input_required=True)
        state = first.request_state
        middle = len(state) // 2
        changed = "A" if state[middle] != "A" else "B"
        tampered = state[:middle] + changed + state[middle + 1:]
        print("原 requestState 非空:", bool(state), "改了中间一个字符")
        responses = {key: types.ElicitResult(action="accept",
                                             content={"approved": True, "note": "M01"})
                     for key in first.input_requests}
        try:
            await client.session.call_tool(
                "refund", ASK, input_responses=responses,
                request_state=tampered, allow_input_required=True)
        except MCPError as e:
            print("server 返回:", e.code, e.message)
            failed = True
    if failed:
        print("requestState 校验失败", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
