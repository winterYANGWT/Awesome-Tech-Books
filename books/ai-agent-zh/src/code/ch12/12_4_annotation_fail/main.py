# mode: fail
# llm: none
# expect: 自动批准
# expect: refund
"""host 策略只看 read_only_hint：谎标的退款工具被自动调用。"""
import asyncio
import json
import os
import sys
import tempfile
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters
from shop import Store

HERE = Path(__file__).resolve().parent


async def main():
    failed = False
    with tempfile.TemporaryDirectory() as td:
        db = str(Path(td) / "shop.db")
        Store(db).db.close()                     # 先建库，server 和本进程共用这个文件
        params = StdioServerParameters(
            command=sys.executable, args=[str(HERE / "server.py")],
            env={"PYTHONPATH": os.environ.get("PYTHONPATH", ""), "SHOP_DB": db})
        async with Client(params) as client:
            tools = (await client.list_tools()).tools
            lying = next(t for t in tools if t.name == "issue_refund")
            print("issue_refund 的注解:", lying.annotations.model_dump(by_alias=True))
            if lying.annotations and lying.annotations.read_only_hint:
                print("策略命中：read_only_hint=True，自动调用，不再问人")
                result = await client.call_tool(
                    "issue_refund", {"order_id": "A1680", "amount": 680, "reason": "装订脱落"})
                print("调用结果:", result.content[0].text)
            else:
                print("策略未命中：转人工确认")
        store = Store(db)
        actions = store.actions()
        print("actions:", json.dumps(actions, ensure_ascii=False))
        if any(a["kind"] == "refund" for a in actions):
            failed = True
    if failed:
        print("自动批准了退款", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
