# mode: run
# llm: none
# expect: input_required
# expect: requestState
# expect: 已退款
"""手动驱动一次 MRTR：拿到 input_required，带答复和原样的 requestState 重试。"""
import argparse
import asyncio
import os
import sys
from pathlib import Path

from mcp import Client, types
from mcp.client.stdio import StdioServerParameters
from mcp.shared.exceptions import MCPError

HERE = Path(__file__).resolve().parent
PARAMS = StdioServerParameters(
    command=sys.executable, args=[str(HERE / "server.py")],
    env={"PYTHONPATH": os.environ.get("PYTHONPATH", "")})
ASK = {"order_id": "A1680", "amount": 680}


async def accept(ctx, params):
    return types.ElicitResult(action="accept", content={"approved": True, "note": "M01"})


async def decline(ctx, params):
    return types.ElicitResult(action="decline")


def trace_tool_calls(client):
    """记下每次 tools/call 的 JSON-RPC id；只为演示，业务代码不用这样。"""
    d = client.session._dispatcher
    ids = []
    original = d.send_raw_request

    async def traced(method, params, opts=None):
        result = await original(method, params, opts)
        if method == "tools/call":
            ids.append(d._next_id)
        return result

    d.send_raw_request = traced
    return ids


# ANCHOR: manual
async def run(callback, answer, label):
    async with Client(PARAMS, elicitation_callback=callback) as client:
        ids = trace_tool_calls(client)
        first = await client.session.call_tool("refund", ASK, allow_input_required=True)
        print(f"== {label} ==")
        print("第一次 resultType:", first.result_type)
        for key, request in (first.input_requests or {}).items():
            print("inputRequests 键:", key)
            print("method:", request.method)
            print("message:", request.params.message)
        print("requestState 非空:", bool(first.request_state),
              "长度 > 300:", len(first.request_state or "") > 300)
        responses = {key: answer for key in (first.input_requests or {})}
        final = await client.session.call_tool(
            "refund", ASK, input_responses=responses,
            request_state=first.request_state, allow_input_required=True)
        print("tools/call 的 id:", " → ".join(map(str, ids)))
        print("重试 resultType:", final.result_type, "is_error:", final.is_error)
        print(final.content[0].text)
# ANCHOR_END: manual


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--decline", action="store_true")
    args = parser.parse_args()
    if not args.decline:
        await run(accept, types.ElicitResult(action="accept",
                                             content={"approved": True, "note": "M01"}), "accept")
    await run(decline, types.ElicitResult(action="decline"), "decline")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except MCPError as e:
        print("MCP 错误:", e.code, e.message)
        raise
