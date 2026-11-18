# llm: none
# expect: /mcp
# expect: pong
# expect: Mcp-Name

"""在同一个事件循环里起 HTTP server，再用 Client 连它。"""

import asyncio
import logging
import socket

import uvicorn
from mcp import Client

import server


def free_port() -> int:
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


async def main() -> None:
    logging.getLogger("httpx").setLevel(logging.WARNING)
    port = free_port()
    config = uvicorn.Config(server.app(), host="127.0.0.1", port=port, log_level="error")
    httpd = uvicorn.Server(config)
    task = asyncio.create_task(httpd.serve())
    while not httpd.started:
        await asyncio.sleep(0.01)

    # ANCHOR: connect
    async with Client(f"http://127.0.0.1:{port}/mcp") as client:   # 一个 URL 就是一个 endpoint
        print("endpoint: /mcp")
        print("tools:", ", ".join(t.name for t in (await client.list_tools()).tools))
        result = await client.call_tool("ping", {"text": "x"})
        print("result:", result.content[0].text)
    # ANCHOR_END: connect

    httpd.should_exit = True
    await task
    print("headers:")
    for row in server.seen:
        print(f"  {row['method']}: MCP-Protocol-Version={row['version']} "
              f"Mcp-Method={row['mcp_method']} Mcp-Name={row['mcp_name'] or '-'}")
        print(f"  Accept: {row['accept']}")
    print("server stopped")


if __name__ == "__main__":
    asyncio.run(main())
