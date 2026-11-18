# mode: run
# expect: 680
# expect: 批准
# expect: refund
"""端到端：模型调退款工具，server 用 MRTR 请主管批准，批准后写进 Store.actions()。"""
import asyncio
import json
import os
import sys
import tempfile
from pathlib import Path
from typing import Annotated

from pydantic import BaseModel, Field
from mcp import Client, types
from mcp.client.stdio import StdioServerParameters
from mcp.server.mcpserver import Elicit, MCPServer, Resolve
from shop import Store

import harness

HERE = Path(__file__).resolve()
DB_ENV = "SHOP_DB"


class Approval(BaseModel):
    approved: bool = Field(description="主管是否批准这笔退款")
    note: str = Field(default="", description="主管编号")


def approval(order_id: str, amount: float) -> Elicit[Approval] | Approval:
    if amount <= 500:
        return Approval(approved=True, note="rule")
    return Elicit(f"批准 {order_id} 退款 {amount:.0f} 元？", Approval)


# ANCHOR: server
def build_server(db: str) -> MCPServer:
    mcp = MCPServer("bookshop-refund")
    store = Store(db)

    @mcp.tool()
    def refund(order_id: str, amount: float,
               ok: Annotated[Approval, Resolve(approval)]) -> dict:
        """给订单退款。单笔超过 500 元时先请主管批准。"""
        if not ok.approved:
            raise ValueError("主管没有批准这笔退款")
        return store.refund(order_id, amount, "装订脱落", approved_by=ok.note)

    return mcp
# ANCHOR_END: server


# ANCHOR: toolbox
class MCPToolbox:
    """把 MCP 的工具表接到 harness：schemas 给模型，call 走 client。"""

    def __init__(self, client, loop):
        self.client = client
        self.loop = loop
        self.tools = {}

    def load(self, tools):
        for tool in tools:
            self.tools[tool.name] = tool

    def schemas(self):
        return [{"type": "function", "function": {
            "name": t.name, "description": t.description or "",
            "parameters": t.input_schema}} for t in self.tools.values()]

    def call(self, name: str, arguments):
        args = json.loads(arguments or "{}") if isinstance(arguments, str) else arguments
        future = asyncio.run_coroutine_threadsafe(
            self.client.call_tool(name, args), self.loop)
        result = future.result()
        text = "\n".join(b.text for b in result.content if b.type == "text")
        return text if not result.is_error else "错误：" + text
# ANCHOR_END: toolbox


# ANCHOR: approve
async def approve(ctx, params):
    print("主管界面:", params.message)
    print("requested_schema 字段:", list(params.requested_schema.get("properties", {})))
    return types.ElicitResult(action="accept",
                              content={"approved": True, "note": "M01"})
# ANCHOR_END: approve


# ANCHOR: run_agent
def run_agent(box):
    messages = [{"role": "user", "content":
                 "主管已经同意订单 A1680 的 680 元全额退款（批准号 M01），"
                 "理由是装订脱落。请直接退款。"}]
    result = harness.run(messages, box, max_turns=4)
    print("回答:", result.answer)
# ANCHOR_END: run_agent


async def main():
    with tempfile.TemporaryDirectory() as td:
        db = str(Path(td) / "shop.db")
        Store(db).db.close()
        params = StdioServerParameters(
            command=sys.executable, args=[str(HERE), "--server"],
            env={"PYTHONPATH": os.environ.get("PYTHONPATH", ""), DB_ENV: db})
        async with Client(params, elicitation_callback=approve) as client:
            box = MCPToolbox(client, asyncio.get_running_loop())
            box.load((await client.list_tools()).tools)
            await asyncio.to_thread(run_agent, box)
        store = Store(db)
        for action in store.actions():
            print("actions:", action["kind"], action["order_id"],
                  json.dumps(action["detail"], ensure_ascii=False))


if __name__ == "__main__":
    if "--server" in sys.argv:
        build_server(os.environ[DB_ENV]).run()
    else:
        asyncio.run(main())
