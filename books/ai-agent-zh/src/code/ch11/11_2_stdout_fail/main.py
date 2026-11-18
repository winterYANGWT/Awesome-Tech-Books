# mode: fail
# llm: none
# expect: stdout
# expect: parse

"""server 的 print 写进 stdout 后，stdio client 读到一行非 JSON。

mcp 2.3 的 client 会记一条 ERROR 后继续；这个示例把那条日志当成失败，
因为线上它意味着协议已经被污染，不能当作正常情况放过。
"""

import asyncio
import logging
import sys
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters

SERVER = Path(__file__).with_name("server.py")


class Watch(logging.Handler):
    def __init__(self) -> None:
        super().__init__()
        self.bad: list[str] = []

    def emit(self, record: logging.LogRecord) -> None:
        if "Failed to parse JSONRPC message" in record.getMessage():
            self.bad.append(record.getMessage())


async def main() -> None:
    watch = Watch()
    logging.getLogger("mcp.client.stdio").addHandler(watch)

    params = StdioServerParameters(command=sys.executable, args=[str(SERVER)])
    async with Client(params) as client:
        print("calling ping")
        result = await client.call_tool("ping", {"text": "x"})
        print("result:", result.content[0].text)

    if watch.bad:                  # 坏行被 SDK 吞掉了，这里主动让它变成失败
        raise SystemExit("MCP stdout 被污染：Failed to parse JSONRPC message from server")


if __name__ == "__main__":
    asyncio.run(main())
